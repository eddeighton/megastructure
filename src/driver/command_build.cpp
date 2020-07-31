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

#include "egcomponent/generator.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "eg_compiler/allocator.hpp"
#include "eg_compiler/sessions/parser_session.hpp"
#include "eg_compiler/sessions/interface_session.hpp"
#include "eg_compiler/sessions/operations_session.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/timer/timer.hpp>
#include <boost/tokenizer.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <iostream>
#include <memory>
#include <map>

/*
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
};*/


void build_include_header( const Environment& environment, const eg::interface::Root* pInterfaceRoot, const ProjectTree& projectTree, const std::string& strCompilationFlags  )
{
	//generate the includes header
	{
		//careful to only write to the file if it has been modified
		VERIFY_RTE( pInterfaceRoot );
		std::ostringstream osInclude;
		eg::generateIncludeHeader( osInclude, 
			pInterfaceRoot, 
			projectTree.getSystemIncludes(), 
			projectTree.getUserIncludes( environment ) );
		boost::filesystem::updateFileIfChanged( 
			projectTree.getIncludeHeader(), osInclude.str() );
		
		/*if( bNoReUsePCH )
		{
			//force the file timestamp to have changed to prevent reusing the pch
			boost::filesystem::last_write_time( projectTree.getIncludeHeader(),
				boost::filesystem::last_write_time( projectTree.getIncludeHeader() ) + 1 );
		}*/
	}
	
	{
		//LogEntry log( std::cout, "Compiling include precompiled header", bBenchCommands );
		
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << strCompilationFlags << " ";
		
		osCmd << "-Xclang -emit-pch -o " << environment.printPath( projectTree.getIncludePCH() ) << " ";
		
		osCmd << environment.printPath( projectTree.getIncludeHeader() ) << " ";
		
		osCmd << "-I " << environment.getEGLibraryInclude().generic_string() << " ";
		
		for( const boost::filesystem::path& includeDirectory : projectTree.getIncludeDirectories( environment ) )
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
				//boost::filesystem::last_write_time( projectTree.getIncludePCH(), 
				//	boost::filesystem::last_write_time( projectTree.getIncludeHeader() ) );
			}
		}
	}
	
}

void build_interface_header( const Environment& environment, eg::ParserSession* pParserSession, const ProjectTree& projectTree, const std::string& strCompilationFlags )
{
	
    //generate the interface
    {
        //LogEntry log( std::cout, "Generating Interface", bBenchCommands );
        VERIFY_RTE( pParserSession );
        std::ostringstream osInterface;
        eg::generateInterface( osInterface, 
            pParserSession->getTreeRoot(), pParserSession->getIdentifiers() );
        boost::filesystem::updateFileIfChanged( projectTree.getInterfaceHeader(), osInterface.str() );
    }
	
	{
		//LogEntry log( std::cout, "Compiling interface to pch", bBenchCommands );
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << strCompilationFlags << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( projectTree.getIncludePCH() ) << " ";
		
		osCmd << "-Xclang -emit-pch -o " << environment.printPath( projectTree.getInterfacePCH() ) << " ";
		osCmd << "-Xclang -egdb=" << environment.printPath( projectTree.getParserDatabaseFile() ) << " ";
		osCmd << "-Xclang -egdll=" << environment.printPath( environment.getClangPluginDll() ) << " ";
		
		osCmd << "-I " << environment.getEGLibraryInclude().generic_string() << " ";
		
		osCmd << environment.printPath( projectTree.getInterfaceHeader() ) << " ";
		
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

void build_parser_session( const Environment& environment, const ProjectTree& projectTree, const std::string& strCompilationFlags )
{
	
	eg::ParserDiagnosticSystem diagnostics( projectTree.getSourceFolder(), std::cout );
	
	eg::ParserSession::SourceCodeTree sourceCodeTree;
	{
		sourceCodeTree.root = projectTree.getSourceFolder();
		projectTree.getSourceFilesMap( sourceCodeTree.files );
	}
	
	{
		std::unique_ptr< eg::ParserSession > pParserSession = 
			std::make_unique< eg::ParserSession >();
			
		pParserSession->parse( sourceCodeTree, diagnostics );
		
		pParserSession->buildAbstractTree();
		
		pParserSession->store( projectTree.getParserDatabaseFile() );
		
		std::cout << "Generated: " << projectTree.getParserDatabaseFile() << std::endl;
		std::cout << std::endl;
		
		const eg::interface::Root* pInterfaceRoot = pParserSession->getTreeRoot();
		
		build_include_header( environment, pInterfaceRoot, projectTree, strCompilationFlags );
		build_interface_header( environment, pParserSession.get(), projectTree, strCompilationFlags );
	}
	
	{
		//LogEntry log( std::cout, "Performing interface analysis", bBenchCommands );
		
		std::unique_ptr< eg::InterfaceSession > pInterfaceSession
			= std::make_unique< eg::InterfaceSession >( projectTree.getParserDatabaseFile() );
			
		//perform the analysis
		pInterfaceSession->instanceAnalysis();
		pInterfaceSession->linkAnalysis();
		
		pInterfaceSession->translationUnitAnalysis( projectTree.getSourceFolder(), 
			[ &projectTree ]( const std::string& strName )
			{
				eg::IndexedObject::FileID fileID = eg::IndexedObject::NO_FILE;
				
				const boost::filesystem::path dbPath = projectTree.getTUDBName( strName );
				if( boost::filesystem::exists( dbPath ) )
				{
					//read the fileID out of the file
					fileID = eg::IndexedFile::readFileID( dbPath );
				}
				
				return fileID;
			}
		);
		
		pInterfaceSession->store( projectTree.getInterfaceDatabaseFile() );
	}
}

void build_operations( eg::InterfaceSession& interfaceSession, const Environment& environment, 
    const ProjectTree& projectTree, const std::string& strCompilationFlags )
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
            boost::filesystem::updateFileIfChanged( projectTree.getOperationsHeader( strTUName ), osOperations.str() );
        }
            
        //compile the operations to pch file
        /*if( fileTracker.isModified( projectTree.getIncludePCH() ) ||
            fileTracker.isModified( projectTree.getInterfacePCH() ) ||
            fileTracker.isModified( projectTree.getOperationsPCH( strTUName ) ) ||
            fileTracker.isModified( projectTree.getOperationsHeader( strTUName ) ) ||
            //fileTracker.isModified( projectTree.getInterfaceDatabaseFile() ) || 
            fileTracker.isModified( projectTree.getTUDBName( strTUName ) ) )
        {*/
            //LogEntry log( std::cout, "Compiling operations to pch: " + strTUName, bBenchCommands );
            
            std::ostringstream osCmd;
            environment.startCompilationCommand( osCmd );
            osCmd << " " << strCompilationFlags << " ";
            
            osCmd << "-Xclang -include-pch ";
            osCmd << "-Xclang " << environment.printPath( projectTree.getIncludePCH() ) << " ";
            
            osCmd << "-Xclang -include-pch ";
            osCmd << "-Xclang " << environment.printPath( projectTree.getInterfacePCH() ) << " ";
            
            osCmd << "-Xclang -emit-pch -o " << environment.printPath( projectTree.getOperationsPCH( strTUName ) ) << " ";
            osCmd << "-Xclang -egdb=" << environment.printPath( projectTree.getInterfaceDatabaseFile() ) << " ";
            osCmd << "-Xclang -egdll=" << environment.printPath( environment.getClangPluginDll() ) << " ";
            
            osCmd << "-Xclang -egtu=" << environment.printPath( projectTree.getTUDBName( strTUName ) ) << " ";
            osCmd << "-Xclang -egtuid=" << pTranslationUnit->getDatabaseFileID() << " ";
            
            osCmd << environment.printPath( projectTree.getOperationsHeader( strTUName ) ) << " ";
            
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


void generate_objects( const eg::TranslationUnitAnalysis& translationUnits, const Environment& environment,
    const ProjectTree& projectTree, const std::string& strCompilationFlags )
{
    eg::IndexedFile::FileIDtoPathMap allFiles;
    
    //use the interface session to determine the files
    {
        allFiles.insert( std::make_pair( eg::IndexedObject::MASTER_FILE, projectTree.getInterfaceDatabaseFile() ) );
        
        //load all the translation unit analysis files
        for( const eg::TranslationUnit* pTranslationUnit : translationUnits.getTranslationUnits() )
        {
            allFiles.insert( std::make_pair( 
				pTranslationUnit->getDatabaseFileID(), 
				projectTree.getTUDBName( pTranslationUnit->getName() ) ) );
        }
    }
    
    {
        //perform the full program analysis
        std::unique_ptr< eg::ImplementationSession > pImplementationSession = 
            std::make_unique< eg::ImplementationSession >( allFiles );
                
        pImplementationSession->fullProgramAnalysis();
        pImplementationSession->store( projectTree.getAnalysisFileName() );
        
    }
    
    
    /*std::ostringstream osPackages;
    bool bHasPackages = false;
    {
        const std::vector< std::string > packages = projectTree.getPackages();
        std::copy( packages.begin(), packages.end(),
            std::ostream_iterator< std::string >( osPackages, " " ) );
        bHasPackages = !packages.empty();
    }
    
    //executing all commands
    std::vector< boost::filesystem::path > commands =  projectTree.getCommands();
    for( const boost::filesystem::path& strCommand : commands )
    {
        std::ostringstream os;
        os << "Executing command: " << strCommand;
        LogEntry log( std::cout, os.str(), bBenchCommands );
        
        std::ostringstream osCmd;
        osCmd << strCommand << " ";
        
        osCmd << "--name " << projectTree.getProject().Name() << " ";
        osCmd << "--database " << projectTree.getAnalysisFileName() << " ";
        osCmd << "--dir " << projectTree.getIntermediateFolder().generic_string() << " ";
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

void generateMegaBufferStructures( std::ostream& os, const eg::ReadSession& program, const ProjectTree& projectTree )
{
    const eg::Layout& layout = program.getLayout();
    
    eg::generateIncludeGuard( os, "STRUCTURES" );
    
    os << "//data structures\n";
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        os << "\n//Buffer: " << pBuffer->getTypeName();
        if( const eg::concrete::Action* pAction = pBuffer->getAction() )
        {
            os << " type: " << pAction->getIndex();
        }
        
        os << /*" stride: " << pBuffer->getStride() <<*/ " size: " << pBuffer->getSize() << "\n";
        os << "struct " << pBuffer->getTypeName() << "\n{\n";
        for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
        {
            os << "    ";
            eg::generateDataMemberType( os, pDataMember );
            os << " " << pDataMember->getName() << ";\n";
        }
        
        os << "};\n";
        os << "extern " << pBuffer->getTypeName() << " *" << pBuffer->getVariableName() << ";\n";
        
    }
    
    os << "\n" << eg::pszLine << eg::pszLine;
    os << "#endif\n";
}

void generateMegaStructureNetStateHeader( std::ostream& os, const eg::ReadSession& program, 
        const ProjectTree& projectTree, const megastructure::NetworkAnalysis& networkAnalysis )
{
    const eg::Layout& layout = program.getLayout();
    
	const std::string& strCoordinatorName   = projectTree.getCoordinatorName();
	const std::string& strHostName          = projectTree.getHostName();
	const std::string& strProjectName       = projectTree.getProjectName();
    
    eg::generateIncludeGuard( os, "NETSTATE" );
    os << "\n//network state data\n";
    
    os << "extern std::bitset< " << networkAnalysis.getReadBitSetSize() << " > g_reads;\n";
    
    const megastructure::NetworkAnalysis::HostStructureMap& hostStructures = 
        networkAnalysis.getHostStructures();
    
    //generate the host id enum
    os << "enum mega_HostID\n{\n";
    for( const auto& i : hostStructures )
    {
        os << "    " << i.second.strIdentityEnumName << "_read,\n";
        os << "    " << i.second.strIdentityEnumName << "_write,\n";
    }
    os << "    g_TotalHostLocks\n";
    os << "};\n";
    os << "extern std::bitset< g_TotalHostLocks > g_hostLocks;\n";
    
    //generate all externs
    for( const auto& i : hostStructures )
    {
        os << "extern std::set< eg::TypeInstance > " << i.second.strWriteSetName << ";\n";
    }
        
    os << "\n" << eg::pszLine << eg::pszLine;
    os << "#endif\n";
}

extern void generatePythonBindings( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateUnrealInterface( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateUnrealCode( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );

void build_component( const eg::ReadSession& session, const Environment& environment,
    const ProjectTree& projectTree, const boost::filesystem::path& binPath, 
    const std::string& strCompilationFlags, const std::vector< std::string >& inputSourceFileNameSet )
{
	bool bBenchCommands = false;
	
	const eg::TranslationUnitAnalysis& translationUnits =
		session.getTranslationUnitAnalysis();
    
    megastructure::NetworkAnalysis networkAnalysis( session, projectTree );
    
    eg::PrinterFactory::Ptr pPrinterFactory = 
        networkAnalysis.getMegastructurePrinterFactory();
        
    megastructure::InstructionCodeGeneratorFactoryImpl 
        instructionCodeGenFactory( networkAnalysis, translationUnits, projectTree );
	
	std::vector< boost::filesystem::path > sourceFiles;
    
    sourceFiles.push_back( projectTree.getCoroutineFrameSourceFilePath( environment ) );
    sourceFiles.push_back( projectTree.getBasicSchedulerFilePath( environment ) );
    sourceFiles.push_back( projectTree.getBasicClockFilePath( environment ) );
	
	//generate the structures
	{
		//LogEntry log( std::cout, "Compiling data structures", bBenchCommands );
		std::ostringstream osStructures;
		generateMegaBufferStructures( osStructures, session, projectTree );
		boost::filesystem::updateFileIfChanged( projectTree.getDataStructureSource(), osStructures.str() );
	}
    
    //generate the netstate header
    {
		std::ostringstream osNetState;
		generateMegaStructureNetStateHeader( osNetState, session, projectTree, networkAnalysis );
		boost::filesystem::updateFileIfChanged( projectTree.getNetStateSource(), osNetState.str() );
    }
	
	//generate the runtime code
	{
		std::ostringstream osImpl;
		osImpl << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
        osImpl << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
		//eg::generate_dynamic_interface( osImpl, *pPrinterFactory, session );
		eg::generateActionInstanceFunctions( osImpl, *pPrinterFactory, session );
		boost::filesystem::updateFileIfChanged( projectTree.getRuntimeSource(), osImpl.str() );
		sourceFiles.push_back( projectTree.getRuntimeSource() );
	}
    
    const Project& project = projectTree.getProject();
    const std::string& strProjectType = project.getHost().Type();
    
	//generate the eg component interface implementation
    if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Basic ] )
    {
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Basic );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Python ] )
    {
        //generate python bindings
		std::ostringstream osPython;
        generatePythonBindings( osPython, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getPythonSource(), osPython.str() );
		sourceFiles.push_back( projectTree.getPythonSource() );
        
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Python );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Unreal ] )
    {
        //generate the unreal interface
		std::ostringstream osUnrealInterface;
        generateUnrealInterface( osUnrealInterface, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getUnrealInterface(), osUnrealInterface.str() );
        
        //generate the unreal implementation
		std::ostringstream osUnreal;
        generateUnrealCode( osUnreal, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getUnrealSource(), osUnreal.str() );
		sourceFiles.push_back( projectTree.getUnrealSource() );
        
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Unreal );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else
    {
        THROW_RTE( "Unknown project type: " << strProjectType );
    }
	
    std::mutex logMutex;
    std::vector< std::function< void() > > commands;
	
	for( const boost::filesystem::path& strSourceFile : sourceFiles )
    {
        //const boost::filesystem::path strSourceFile = projectTree.getRuntimeSource();
        boost::filesystem::path objectFilePath = projectTree.getObjectFile( strSourceFile, binPath );
        //objectFiles.push_back( objectFilePath );
        
        /*if( fileTracker.isModified( projectTree.getIncludePCH() ) ||
            fileTracker.isModified( projectTree.getInterfacePCH() ) ||
            fileTracker.isModified( strSourceFile ) ||
            fileTracker.isModified( objectFilePath ) )
        {*/
		std::ostringstream os;
		os << "Compiling: " << objectFilePath.generic_string();
			
		std::ostringstream osCmd;
		environment.startCompilationCommand( osCmd );
		osCmd << " " << strCompilationFlags << " ";
		
		osCmd << "-c -o " << environment.printPath( objectFilePath ) << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( projectTree.getIncludePCH() ) << " ";
		
		osCmd << "-Xclang -include-pch ";
		osCmd << "-Xclang " << environment.printPath( projectTree.getInterfacePCH() ) << " ";
	
		osCmd << "-I " << environment.printPath( environment.getEGLibraryInclude() ) << " ";
		osCmd << "-I " << environment.printPath( projectTree.getInterfaceFolder() ) << " ";
		
		for( const boost::filesystem::path& includeDirectory : projectTree.getIncludeDirectories( environment ) )
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
    
    std::vector< std::string > additionalIncludes = 
    { 
        projectTree.getStructuresInclude(), 
        projectTree.getNetStateSourceInclude() 
    };
    
	//generate the implementation files for the coordinator host
    for( const eg::TranslationUnit* pTranslationUnit : translationUnits.getTranslationUnits() )
    {
		if(     std::find( inputSourceFileNameSet.begin(), inputSourceFileNameSet.end(), 
                    pTranslationUnit->getName() ) != inputSourceFileNameSet.end() ||
                ( 
                    pTranslationUnit->getCoordinatorHostnameDefinitionFile().isCoordinator( projectTree.getCoordinatorName() ) &&
                    pTranslationUnit->getCoordinatorHostnameDefinitionFile().isHost( projectTree.getHostName() ) 
                ) 
            )
		{
			//generate the implementation source code
			{
				//LogEntry log( std::cout, "Generating implementation: " + pTranslationUnit->getName(), bBenchCommands );
				std::ostringstream osImpl;
				eg::generateImplementationSource( osImpl, instructionCodeGenFactory, *pPrinterFactory, session, *pTranslationUnit, additionalIncludes );
				boost::filesystem::updateFileIfChanged( projectTree.getImplementationSource( pTranslationUnit->getName() ), osImpl.str() );
			}
			
			boost::filesystem::path objectFilePath = projectTree.getObjectName( pTranslationUnit->getName(), binPath );
			//objectFiles.push_back( objectFilePath );
			
			/*if( fileTracker.isModified( projectTree.getIncludePCH() ) ||
				fileTracker.isModified( projectTree.getInterfacePCH() ) ||
				fileTracker.isModified( projectTree.getOperationsPCH( pTranslationUnit->getName() ) ) ||
				fileTracker.isModified( objectFilePath ) )
			{*/
				std::ostringstream os;
				os << "Compiling: " << objectFilePath.generic_string();
				
				std::ostringstream osCmd;
				environment.startCompilationCommand( osCmd );
				osCmd << " " << strCompilationFlags << " ";
				
				osCmd << "-c -o " << environment.printPath( objectFilePath ) << " ";
					
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( projectTree.getIncludePCH() ) << " ";
				
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( projectTree.getInterfacePCH() ) << " ";
				
				osCmd << "-Xclang -include-pch ";
				osCmd << "-Xclang " << environment.printPath( projectTree.getOperationsPCH( pTranslationUnit->getName() ) ) << " ";
					
                for( const boost::filesystem::path& includeDirectory : projectTree.getImplIncludeDirectories( environment ) )
                {
                    osCmd << "-I " << environment.printPath( includeDirectory ) << " ";
                }
                
				osCmd << environment.printPath( projectTree.getImplementationSource( pTranslationUnit->getName() ) ) << " ";
					
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
	std::vector< std::string > flags;
    std::string names;
    
    namespace po = boost::program_options;
    po::options_description commandOptions(" Build Project Command");
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
			("flags",   	po::value< std::vector< std::string > >( &flags ), "C++ Compilation Flags" )
			("names",   	po::value< std::string >( &names ), "eg source file names ( no extension, semicolon delimited )" )
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
		else
		{
			std::cout << "Got build command: " << strBuildCommand << std::endl;
		}
		
		std::string strCompilationFlags;
		{
			std::ostringstream osFlags;
			for( const auto& str : flags )
				osFlags << str << " ";
			strCompilationFlags = osFlags.str();
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
			
			build_parser_session( environment, projectTree, strCompilationFlags );
			
			std::unique_ptr< eg::InterfaceSession > pInterfaceSession
				 = std::make_unique< eg::InterfaceSession >( projectTree.getInterfaceDatabaseFile() );
				 
			build_operations( *pInterfaceSession, environment, projectTree, strCompilationFlags );
			
			generate_objects( pInterfaceSession->getTranslationUnitAnalysis(), environment, projectTree, strCompilationFlags );
			
		}
		else
		{
        
            //tokenize semi colon delimited names
            std::vector< std::string > egFileNames;
            {
                using Tokeniser = boost::tokenizer< boost::char_separator< char > >;
                boost::char_separator< char > sep( ";" );
                Tokeniser tokens( names, sep );
                for ( Tokeniser::iterator i = tokens.begin(); i != tokens.end(); ++i )
                    egFileNames.push_back( *i );
            }
            for( const std::string& str : egFileNames )
            {
                std::cout << "eg source name: " << str << std::endl;
            }
        
			VERIFY_RTE_MSG( !strCoordinator.empty(), "Missing Coordinator" );
			VERIFY_RTE_MSG( !strHost.empty(), "Missing Host Name" );
			
			std::cout << "building component: " << strCoordinator << " " << strHost << " " << strProject << std::endl;
			
			ProjectTree projectTree( environment, projectDirectory, strCoordinator, strHost, strProject );
			
			eg::ReadSession session( projectTree.getAnalysisFileName() );
			
			build_component( session, environment, projectTree, binPath, strCompilationFlags, egFileNames );
					
		}
    }
    
}
