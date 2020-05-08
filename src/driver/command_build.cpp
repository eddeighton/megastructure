//  Copyright (c) Deighton Systems Limited. 2019. All Rights Reserved.
//  Author: Edward Deighton
//  License: Please see license.txt in the project root folder.

//  Use and copying of this software and preparation of derivative works
//  based upon this software are permitted. Any copy of this software or
//  of any derivative work must include the above copyright notice, this
//  paragraph and the one after it.  Any distribution of this software or
//  derivative works must comply with all applicable laws.

//  This software is made available AS IS, and COPYRIGHT OWNERS DISCLAIMS
//  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
//  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
//  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
//  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF COPYRIGHT OWNERS ARE ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGES.


#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "eg_compiler/sessions/parser_session.hpp"
#include "eg_compiler/sessions/interface_session.hpp"
#include "eg_compiler/sessions/operations_session.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <iostream>
#include <memory>
#include <map>


struct LogEntry
{
private:
    std::ostream& os;
    const std::string msg;
    boost::timer::cpu_timer timer_internal;
    bool bBenchCommands;
public:
    LogEntry( std::ostream& os, const std::string& pszMsg, bool bBenchCommands )
        :   os( os ),
            msg( pszMsg ),
            bBenchCommands( bBenchCommands )
    {
    }
    
    ~LogEntry()
    {
        if( bBenchCommands )
            os << timer_internal.format( 3, "%w seconds" ) << ": " << msg << "\n";
    }
};

class FileWriteTracker
{
    using FileTimeMap = std::map< boost::filesystem::path, std::time_t >;
public:

    FileWriteTracker( const Project& project )
    {
        const boost::filesystem::path& buildFolder = project.getIntermediateFolder();
        if( boost::filesystem::exists( buildFolder ) )
        {
            for( boost::filesystem::directory_iterator itr( buildFolder );
                itr != boost::filesystem::directory_iterator(); ++itr )
            {
                if( boost::filesystem::is_regular_file( itr->status() ) )
                    recordFile( itr->path() );
            }
        }
    }

    bool isModified( const boost::filesystem::path& filePath ) const
    {
        FileTimeMap::const_iterator iFind = m_fileTimes.find( filePath );
        if( iFind == m_fileTimes.end() )
        {
            return true;
        }
        else
        {
            return iFind->second != boost::filesystem::last_write_time( filePath );
        }
    }
    
private:
    void recordFile( const boost::filesystem::path& filePath )
    {
        ASSERT( m_fileTimes.find( filePath ) == m_fileTimes.end() );
        m_fileTimes.insert( std::make_pair( filePath, boost::filesystem::last_write_time( filePath ) ) );
    }
    
    FileTimeMap m_fileTimes;
};


void build_include_header( const Environment& environment, const eg::interface::Root* pInterfaceRoot, const ProjectTree& project )
{
	//generate the includes header
	{
		//careful to only write to the file if it has been modified
		VERIFY_RTE( pInterfaceRoot );
		std::ostringstream osInclude;
		eg::generateIncludeHeader( osInclude, 
			pInterfaceRoot, 
			project.getSystemIncludes(), 
			project.getUserIncludes( environment ) );
		boost::filesystem::updateFileIfChanged( 
			project.getIncludeHeader(), osInclude.str() );
		
		/*if( bNoReUsePCH )
		{
			//force the file timestamp to have changed to prevent reusing the pch
			boost::filesystem::last_write_time( project.getIncludeHeader(),
				boost::filesystem::last_write_time( project.getIncludeHeader() ) + 1 );
		}*/
	}
	
	{
		//LogEntry log( std::cout, "Compiling include precompiled header", bBenchCommands );
		
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << project.getCompilerFlags() << " ";
		
		osCmd << "-Xclang -emit-pch -o " << environment.printPath( project.getIncludePCH() ) << " ";
		
		osCmd << environment.printPath( project.getIncludeHeader() ) << " ";
		
		osCmd << "-I " << environment.getEGLibraryInclude().generic_string() << " ";
		
		for( const boost::filesystem::path& includeDirectory : project.getIncludeDirectories( environment ) )
		{
			osCmd << "-I " << environment.printPath( includeDirectory ) << " ";
		}
		
		//if( bLogCommands )
		//{
			std::cout << "\n" << osCmd.str() << std::endl;
		//}
		
		{
			const int iResult = boost::process::system( osCmd.str() );
			if( iResult )
			{
				THROW_RTE( "Error invoking clang++ " << iResult );
			}
			else
			{
				//artificially set the file time stamp to match the include file
				//boost::filesystem::last_write_time( project.getIncludePCH(), 
				//	boost::filesystem::last_write_time( project.getIncludeHeader() ) );
			}
		}
	}
	
}

void build_interface_header( const Environment& environment, eg::ParserSession* pParserSession, const ProjectTree& project )
{
	
    //generate the interface
    {
        //LogEntry log( std::cout, "Generating Interface", bBenchCommands );
        VERIFY_RTE( pParserSession );
        std::ostringstream osInterface;
        eg::generateInterface( osInterface, 
            pParserSession->getTreeRoot(), pParserSession->getIdentifiers(), project.getFiberStackSize() );
        boost::filesystem::updateFileIfChanged( project.getInterfaceHeader(), osInterface.str() );
    }
	
	{
		//LogEntry log( std::cout, "Compiling interface to pch", bBenchCommands );
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << project.getCompilerFlags() << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( project.getIncludePCH() ) << " ";
		
		osCmd << "-Xclang -emit-pch -o " << environment.printPath( project.getInterfacePCH() ) << " ";
		osCmd << "-Xclang -egdb=" << environment.printPath( project.getParserDatabaseFile() ) << " ";
		osCmd << "-Xclang -egdll=" << environment.printPath( environment.getClangPluginDll() ) << " ";
		
		osCmd << "-I " << environment.getEGLibraryInclude().generic_string() << " ";
		
		osCmd << environment.printPath( project.getInterfaceHeader() ) << " ";
		
		//if( bLogCommands )
		//{
			std::cout << "\n" << osCmd.str() << std::endl;
		//}
		
		{
			const int iResult = boost::process::system( osCmd.str() );
			if( iResult )
			{
				THROW_RTE( "Error invoking clang++ " << iResult );
			}
		}
	}
}

void build_parser_session( const Environment& environment, const ProjectTree& project )
{
	
	eg::ParserDiagnosticSystem diagnostics( project.getRootPath(), std::cout );
	
	eg::ParserSession::SourceCodeTree sourceCodeTree;
	{
		sourceCodeTree.root = project.getRootPath();
		project.getSourceFilesMap( sourceCodeTree.files );
	}
	
	{
		std::unique_ptr< eg::ParserSession > pParserSession = 
			std::make_unique< eg::ParserSession >();
			
		pParserSession->parse( sourceCodeTree, diagnostics );
		
		pParserSession->buildAbstractTree();
		
		pParserSession->store( project.getParserDatabaseFile() );
		
		std::cout << "Generated: " << project.getParserDatabaseFile() << std::endl;
		std::cout << std::endl;
		
		const eg::interface::Root* pInterfaceRoot = pParserSession->getTreeRoot();
		//std::string strIndent;
		//pInterfaceRoot->print( std::cout, strIndent, true );
		
		build_include_header( environment, pInterfaceRoot, project );
		build_interface_header( environment, pParserSession.get(), project );
	}
	
	{
		//LogEntry log( std::cout, "Performing interface analysis", bBenchCommands );
		
		std::unique_ptr< eg::InterfaceSession > pInterfaceSession
			= std::make_unique< eg::InterfaceSession >( project.getParserDatabaseFile() );
			
		//perform the analysis
		pInterfaceSession->linkAnalysis();
		pInterfaceSession->instanceAnalysis();
		pInterfaceSession->dependencyAnalysis();
		
		pInterfaceSession->translationUnitAnalysis( project.getRootPath(), 
			[ &project ]( const std::string& strName )
			{
				eg::IndexedObject::FileID fileID = eg::IndexedObject::NO_FILE;
				
				const boost::filesystem::path dbPath = project.getTUDBName( strName );
				if( boost::filesystem::exists( dbPath ) )
				{
					//read the fileID out of the file
					fileID = eg::IndexedFile::readFileID( dbPath );
				}
				
				return fileID;
			}
		);
		
		pInterfaceSession->store( project.getInterfaceDatabaseFile() );
	}
}

void build_operations( eg::InterfaceSession& interfaceSession, const Environment& environment, 
    const ProjectTree& project /*, FileWriteTracker& fileTracker, bool bBenchCommands, bool bLogCommands*/ )
{
    //interface session MUST NOT store beyond this point - compiler will be loaded TU analysis sessions
    const eg::TranslationUnitAnalysis& tuAnalysis = interfaceSession.getTranslationUnitAnalysis();
    for( const eg::TranslationUnit* pTranslationUnit : tuAnalysis.getTranslationUnits() )
    {
        const std::string& strTUName = pTranslationUnit->getName();
        
        //generate the operation code
        {
            //LogEntry log( std::cout, "Generating operations: " + strTUName, bBenchCommands );
			std::cout << "\nGenerating operations: " << strTUName << std::endl;
            std::ostringstream osOperations;
            eg::generateOperationSource( osOperations, interfaceSession.getTreeRoot(), *pTranslationUnit );
            boost::filesystem::updateFileIfChanged( project.getOperationsHeader( strTUName ), osOperations.str() );
        }
            
        //compile the operations to pch file
        /*if( fileTracker.isModified( project.getIncludePCH() ) ||
            fileTracker.isModified( project.getInterfacePCH() ) ||
            fileTracker.isModified( project.getOperationsPCH( strTUName ) ) ||
            fileTracker.isModified( project.getOperationsHeader( strTUName ) ) ||
            //fileTracker.isModified( project.getInterfaceDatabaseFile() ) || 
            fileTracker.isModified( project.getTUDBName( strTUName ) ) )
        {*/
            //LogEntry log( std::cout, "Compiling operations to pch: " + strTUName, bBenchCommands );
            
            std::ostringstream osCmd;
            environment.startCompilationCommand( osCmd );
            osCmd << " " << project.getCompilerFlags() << " ";
            
            osCmd << "-Xclang -include-pch ";
            osCmd << "-Xclang " << environment.printPath( project.getIncludePCH() ) << " ";
            
            osCmd << "-Xclang -include-pch ";
            osCmd << "-Xclang " << environment.printPath( project.getInterfacePCH() ) << " ";
            
            osCmd << "-Xclang -emit-pch -o " << environment.printPath( project.getOperationsPCH( strTUName ) ) << " ";
            osCmd << "-Xclang -egdb=" << environment.printPath( project.getInterfaceDatabaseFile() ) << " ";
            osCmd << "-Xclang -egdll=" << environment.printPath( environment.getClangPluginDll() ) << " ";
            
            osCmd << "-Xclang -egtu=" << environment.printPath( project.getTUDBName( strTUName ) ) << " ";
            osCmd << "-Xclang -egtuid=" << pTranslationUnit->getDatabaseFileID() << " ";
            
            osCmd << environment.printPath( project.getOperationsHeader( strTUName ) ) << " ";
            
            //if( bLogCommands )
            //{
                std::cout << "\n" << osCmd.str() << std::endl;
            //}
            
            {
                const int iResult = boost::process::system( osCmd.str() );
                if( iResult )
                {
                    THROW_RTE( "Error invoking clang++ " << iResult );
                }
            }
        //}
    }
}

extern void generate_eg_component( std::ostream& os, 
		const std::string& strProjectName, 
		const std::string& strCoordinator, 
		const std::string& strHost, 
		const eg::ReadSession& session );

void generate_objects( const eg::TranslationUnitAnalysis& translationUnits, const Environment& environment,
    const ProjectTree& project /*, FileWriteTracker& fileTracker, bool bBenchCommands, bool bLogCommands*/ )
{
    eg::IndexedFile::FileIDtoPathMap allFiles;
    
    //use the interface session to determine the files
    {
        allFiles.insert( std::make_pair( eg::IndexedObject::MASTER_FILE, project.getInterfaceDatabaseFile() ) );
        
        //load all the translation unit analysis files
        for( const eg::TranslationUnit* pTranslationUnit : translationUnits.getTranslationUnits() )
        {
            allFiles.insert( std::make_pair( 
				pTranslationUnit->getDatabaseFileID(), 
				project.getTUDBName( pTranslationUnit->getName() ) ) );
        }
    }
    
    {
        //perform the full program analysis
        std::unique_ptr< eg::ImplementationSession > pImplementationSession = 
            std::make_unique< eg::ImplementationSession >( allFiles );
                
        pImplementationSession->fullProgramAnalysis();
        pImplementationSession->store( project.getAnalysisFileName() );
        
    }
    
    
    /*std::ostringstream osPackages;
    bool bHasPackages = false;
    {
        const std::vector< std::string > packages = project.getPackages();
        std::copy( packages.begin(), packages.end(),
            std::ostream_iterator< std::string >( osPackages, " " ) );
        bHasPackages = !packages.empty();
    }
    
    //executing all commands
    std::vector< boost::filesystem::path > commands =  project.getCommands();
    for( const boost::filesystem::path& strCommand : commands )
    {
        std::ostringstream os;
        os << "Executing command: " << strCommand;
        LogEntry log( std::cout, os.str(), bBenchCommands );
        
        std::ostringstream osCmd;
        osCmd << strCommand << " ";
        
        osCmd << "--name " << project.getProject().Name() << " ";
        osCmd << "--database " << project.getAnalysisFileName() << " ";
        osCmd << "--dir " << project.getIntermediateFolder().generic_string() << " ";
        if( bHasPackages )
        {
            osCmd << "--package " << osPackages.str() << " ";
        }
        
        //if( bLogCommands )
        //{
        //    std::cout << "\n" << osCmd.str() << std::endl;
        //}
        
        {
            const int iResult = boost::process::system( osCmd.str() );
            if( iResult )
            {
                THROW_RTE( "Error invoking host command " << iResult );
            }
        }
    }*/
}

void objectCompilationCommand( std::string strMsg, std::string strCommand, 
                                bool bBenchCommands, std::mutex& logMutex )
{
    boost::timer::cpu_timer timer_internal;
    
    const int iResult = boost::process::system( strCommand );
    if( iResult )
    {
        THROW_RTE( "Error invoking clang++ with command: " << strCommand << "\n Error code: " << iResult );
    }
    if( bBenchCommands )
    {
        std::lock_guard g( logMutex );
        std::cout << timer_internal.format( 3, "%w seconds" ) << ": " << strMsg << "\n";
    }
}

void objectCompilationCommandSetFileTIme( std::string strMsg, std::string strCommand, 
                                bool bBenchCommands, std::mutex& logMutex, 
                                const boost::filesystem::path strSourceFile, 
                                const boost::filesystem::path strObjectFile )
{
    objectCompilationCommand( strMsg, strCommand, bBenchCommands, logMutex );
    
    boost::filesystem::last_write_time( strObjectFile, 
        boost::filesystem::last_write_time( strSourceFile ) );
}

void build_component( const eg::ReadSession& session, const Environment& environment,
    const ProjectTree& project, const boost::filesystem::path& binPath )
{
	bool bBenchCommands = false;
	
	const eg::TranslationUnitAnalysis& translationUnits =
		session.getTranslationUnitAnalysis();
	
	std::vector< boost::filesystem::path > sourceFiles;
	
	//generate the structures
	{
		//LogEntry log( std::cout, "Compiling data structures", bBenchCommands );
		std::ostringstream osStructures;
		eg::generateBufferStructures( osStructures, session );
		boost::filesystem::updateFileIfChanged( project.getDataStructureSource(), osStructures.str() );
	}
	
	//generate the runtime code
	{
		std::ostringstream osImpl;
		osImpl << "#include \"structures.hpp\"\n";
		eg::generate_dynamic_interface( osImpl, session );
		eg::generateActionInstanceFunctions( osImpl, session );
		boost::filesystem::updateFileIfChanged( project.getRuntimeSource(), osImpl.str() );
		sourceFiles.push_back( project.getRuntimeSource() );
	}
	
	//generate python bindings
	{
		std::ostringstream osCmd;
		osCmd << environment.printPath( environment.expand( "${EG}/bin/python_host.exe" ) ) << " ";
		
		osCmd << "--name " << project.getProjectName() << " ";
		osCmd << "--database " << project.getAnalysisFileName() << " ";
		osCmd << "--dir " << project.getInterfaceFolder().generic_string() << " ";
		osCmd << "--target " << project.getPythonFileName().generic_string() << " ";
		
		{
			std::cout << "\n" << osCmd.str() << std::endl;
			
			const int iResult = boost::process::system( osCmd.str() );
			if( iResult )
			{
				THROW_RTE( "Error invoking python_host.exe " << iResult );
			}
		}
		sourceFiles.push_back( project.getPythonSource() );
	}
	
	//generate the eg component interface implementation
	{
		std::ostringstream osEGComponent;
		generate_eg_component( osEGComponent, project.getProjectName(), project.getCoordinatorName(), project.getHostName(), session );
		const boost::filesystem::path egComponentSourceFilePath = project.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
	}
	
    std::mutex logMutex;
    std::vector< std::function< void() > > commands;
	
	for( const boost::filesystem::path& strSourceFile : sourceFiles )
    {
        //const boost::filesystem::path strSourceFile = project.getRuntimeSource();
        boost::filesystem::path objectFilePath = project.getObjectFile( strSourceFile, binPath );
        //objectFiles.push_back( objectFilePath );
        
        /*if( fileTracker.isModified( project.getIncludePCH() ) ||
            fileTracker.isModified( project.getInterfacePCH() ) ||
            fileTracker.isModified( strSourceFile ) ||
            fileTracker.isModified( objectFilePath ) )
        {*/
		std::ostringstream os;
		os << "Compiling: " << objectFilePath.generic_string();
			
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << project.getCompilerFlags() << " ";
		
		osCmd << "-c -o " << environment.printPath( objectFilePath ) << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( project.getIncludePCH() ) << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( project.getInterfacePCH() ) << " ";
	
		osCmd << "-I " << environment.printPath( environment.getEGLibraryInclude() ) << " ";
		osCmd << "-I " << environment.printPath( project.getInterfaceFolder() ) << " ";
		
		for( const boost::filesystem::path& includeDirectory : project.getIncludeDirectories( environment ) )
		{
			osCmd << "-I " << environment.printPath( includeDirectory ) << " ";
		}
		
		osCmd << environment.printPath( strSourceFile ) << " ";
			
		//if( bLogCommands )
		//{
			std::cout << "\n" << osCmd.str() << std::endl;
		//}
		
		commands.push_back( std::bind( objectCompilationCommand, 
			os.str(), osCmd.str(), bBenchCommands, std::ref( logMutex ) ) );
        //}
    }
	
	//generate the implementation files for the coordinator host
    for( const eg::TranslationUnit* pTranslationUnit : translationUnits.getTranslationUnits() )
    {
		if( pTranslationUnit->getCoordinatorHostnameDefinitionFile().isHost( project.getHostName() ) )
		{
			//generate the implementation source code
			{
				//LogEntry log( std::cout, "Generating implementation: " + pTranslationUnit->getName(), bBenchCommands );
				std::ostringstream osImpl;
				eg::generateImplementationSource( osImpl, session, *pTranslationUnit );
				boost::filesystem::updateFileIfChanged( project.getImplementationSource( pTranslationUnit->getName() ), osImpl.str() );
			}
			
			boost::filesystem::path objectFilePath = project.getObjectName( pTranslationUnit->getName(), binPath );
			//objectFiles.push_back( objectFilePath );
			
			/*if( fileTracker.isModified( project.getIncludePCH() ) ||
				fileTracker.isModified( project.getInterfacePCH() ) ||
				fileTracker.isModified( project.getOperationsPCH( pTranslationUnit->getName() ) ) ||
				fileTracker.isModified( objectFilePath ) )
			{*/
				std::ostringstream os;
				os << "Compiling: " << objectFilePath.generic_string();
				
				std::ostringstream osCmd;
				environment.startCompilationCommand( osCmd );
				osCmd << " " << project.getCompilerFlags() << " ";
				
				osCmd << "-c -o " << environment.printPath( objectFilePath ) << " ";
					
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( project.getIncludePCH() ) << " ";
				
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( project.getInterfacePCH() ) << " ";
				
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( project.getOperationsPCH( pTranslationUnit->getName() ) ) << " ";
					
				osCmd << environment.printPath( project.getImplementationSource( pTranslationUnit->getName() ) ) << " ";
					
				//if( bLogCommands )
				//{
					std::cout << "\n" << osCmd.str() << std::endl;
				//}
				
				commands.push_back( std::bind( objectCompilationCommand, 
					os.str(), osCmd.str(), bBenchCommands, std::ref( logMutex ) ) );
			//}
		}
    }
    
    //brute force attempt to do all compilations at once
    std::vector< std::thread > threads;
    for( std::function< void() >& compilation : commands )
    {
        threads.push_back( std::thread( compilation ) );
    }
    
    for( std::thread& th : threads )
    {
        if( th.joinable() )
            th.join();
    }
}

void command_build( bool bHelp, const std::string& strBuildCommand, const std::vector< std::string >& args )
{
    std::string strDirectory, strProject, strCoordinator, strHost, strBin;
	boost::filesystem::path binPath;
    bool bBenchCommands = false;
    bool bLogCommands = false;
    bool bNoPCH = false;
    bool bFullRebuild = false;
    
    namespace po = boost::program_options;
    po::options_description commandOptions(" Create Project Command");
    {
        commandOptions.add_options()
            ("dir",     	po::value< std::string >( &strDirectory ), "Project directory")
			("project", 	po::value< std::string >( &strProject ), "Project Name" )
			("coordinator", po::value< std::string >( &strCoordinator ), "Coordinator Name" )
			("host", 		po::value< std::string >( &strHost ), "Host Name" )
			("bin",         po::value< boost::filesystem::path >( &binPath ), "Binary Directory" )
            ("bench",   	po::bool_switch( &bBenchCommands ), "Benchmark compilation steps" )
            ("trace",   	po::bool_switch( &bLogCommands ), "Trace compilation commands" )
            ("nopch",   	po::bool_switch( &bNoPCH ), "Force regeneration of precompiled header file" )
            ("full",    	po::bool_switch( &bFullRebuild ), "Full rebuild - do not reuse previous objects or precompiled headers" )
        ;
    }
    
    po::positional_options_description p;
    p.add( "dir", -1 );
    
    po::variables_map vm;
    po::store( po::command_line_parser( args ).options( commandOptions ).positional( p ).run(), vm );
    po::notify( vm );
    
    if( bHelp )
    {
        std::cout << commandOptions << "\n";
    }
    else
    {
        if( strBuildCommand.empty() )
        {
            std::cout << "Missing build command type" << std::endl;
            return;
        }
		
		if( strProject.empty() )
		{
            std::cout << "Missing project name" << std::endl;
            return;
		}
        
        const boost::filesystem::path projectDirectory = 
            boost::filesystem::edsCannonicalise(
                boost::filesystem::absolute( strDirectory ) );

        if( !boost::filesystem::exists( projectDirectory ) )
        {
            THROW_RTE( "Specified directory does not exist: " << projectDirectory.generic_string() );
        }
        else if( !boost::filesystem::is_directory( projectDirectory ) )
        {
            THROW_RTE( "Specified path is not a directory: " << projectDirectory.generic_string() );
        }
		
		//if( bFullRebuild && boost::filesystem::exists( project.getIntermediateFolder() ) )
		//{
		//    std::cout << "Removing: " << project.getIntermediateFolder().generic_string() << std::endl;
		//    boost::filesystem::remove_all( project.getIntermediateFolder() );
		//}
		
		//FileWriteTracker fileTracker( project ); 
		
			
        Environment environment;
		
		if( strCoordinator.empty() && strHost.empty() )
		{
			ProjectTree projectTree( environment, projectDirectory, strProject );
			
			//projectTree.print( std::cout );
			
			build_parser_session( environment, projectTree /*fileTracker, bBenchCommands, bLogCommands, bNoPCH*/ );
			
			std::unique_ptr< eg::InterfaceSession > pInterfaceSession
				 = std::make_unique< eg::InterfaceSession >( projectTree.getInterfaceDatabaseFile() );
				 
			build_operations( *pInterfaceSession, environment, projectTree /*, fileTracker, bBenchCommands, bLogCommands*/ );
			
			generate_objects( pInterfaceSession->getTranslationUnitAnalysis(), environment, projectTree/*, fileTracker, bBenchCommands, bLogCommands*/ );
			
		}
		else
		{
			VERIFY_RTE_MSG( !strCoordinator.empty(), "Missing Coordinator" );
			VERIFY_RTE_MSG( !strHost.empty(), "Missing Host Name" );
			
			std::cout << "building component: " << strCoordinator << " " << strHost << " " << strProject << std::endl;
			
			ProjectTree projectTree( environment, projectDirectory, strCoordinator, strHost, strProject );
			
			eg::ReadSession session( projectDirectory / "interface" / projectTree.getProjectName() / "database.db" );
			
			build_component( session, environment, projectTree, binPath );
					
		}
    }
    
}
