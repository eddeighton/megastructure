
#include "build_interface.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>

#include <iostream>
#include <memory>
#include <map>

static DiagnosticsConfig m_config;

void Task_ParserSession::run()
{
    START_BENCHMARK( "Task_ParserSession: " << m_projectTree.getParserDatabaseFile() );
        
    eg::ParserSession::SourceCodeTree sourceCodeTree;
    {
        sourceCodeTree.root = m_projectTree.getSourceFolder();
        m_projectTree.getSourceFilesMap( sourceCodeTree.files );
    }
    
    {
        m_session_parser = 
            std::make_unique< eg::ParserSession >( m_environment.getParserDll(),
                m_projectTree.getSourceFolder(), std::cout );
        m_session_parser->parse( sourceCodeTree );
        m_session_parser->buildAbstractTree();
        m_session_parser->store( m_projectTree.getParserDatabaseFile() );
    }
}

void Task_ParserSessionCopy::run()
{
    START_BENCHMARK( "Task_ParserSessionCopy: " << m_projectTree.getComponentParserDatabase( m_component ) );
    
    const boost::filesystem::path sourceFile = m_projectTree.getParserDatabaseFile();
    const boost::filesystem::path targetFile = m_projectTree.getComponentParserDatabase( m_component );
    
    VERIFY_RTE( boost::filesystem::exists( sourceFile ) );
    
    if( boost::filesystem::exists( targetFile ) )
    {
        boost::filesystem::remove( targetFile );
    }
    
    boost::filesystem::ensureFoldersExist( targetFile );
    boost::filesystem::copy( sourceFile, targetFile );
}

void Task_MainIncludePCH::run()
{
    START_BENCHMARK( "Task_MainIncludePCH: " << m_environment.printPath( m_projectTree.getIncludePCH() ) );
    
    VERIFY_RTE( m_session_parser );
    
    const eg::interface::Root* pInterfaceRoot = m_session_parser->getTreeRoot();
    
    //generate the includes header
    {
        //careful to only write to the file if it has been modified
        VERIFY_RTE( pInterfaceRoot );
        std::ostringstream osInclude;
        eg::generateIncludeHeader( osInclude, 
            pInterfaceRoot, 
            m_projectTree.getSystemIncludes(), 
            m_projectTree.getUserIncludes( m_environment ) );
        boost::filesystem::updateFileIfChanged( 
            m_projectTree.getIncludeHeader(), osInclude.str() );
    }
    
    build::Compilation cmd( m_projectTree.getIncludeHeader(), 
                            m_projectTree.getIncludePCH(), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.includeDirs = m_projectTree.getIncludeDirectories( m_environment );
        cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
    }
    
    invokeCachedCompiler( m_environment, cmd );
}

void Task_MainInterfacePCH::run()
{
    START_BENCHMARK( "Task_MainInterfacePCH: " << m_environment.printPath( m_projectTree.getInterfacePCH() ) );
        
    VERIFY_RTE( m_session_parser );
    
    const eg::interface::Root* pInterfaceRoot = m_session_parser->getTreeRoot();
    
    //generate the interface
    {
        std::ostringstream osInterface;
        eg::generateInterface( osInterface, 
            m_session_parser->getTreeRoot(), m_session_parser->getIdentifiers() );
        boost::filesystem::updateFileIfChanged( m_projectTree.getInterfaceHeader(), osInterface.str() );
    }
    
    
    build::Compilation cmd( m_projectTree.getInterfaceHeader(), 
                            m_projectTree.getInterfacePCH(), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.egdb = m_projectTree.getParserDatabaseFile();
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_InterfaceSession::run()
{
    START_BENCHMARK( "Task_InterfaceSession: " );
    
    VERIFY_RTE( m_session_parser );
    
    m_session_interface = std::make_unique< eg::InterfaceSession >( 
        m_projectTree.getParserDatabaseFile() );
        
    //perform the analysis
    m_session_interface->instanceAnalysis();
    m_session_interface->linkAnalysis();
    
    const ProjectTree& projectTree = m_projectTree;
    
    m_session_interface->translationUnitAnalysis( m_projectTree.getSourceFolder(), 
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
    
    m_session_interface->store( projectTree.getInterfaceDatabaseFile() );
}

void Task_MainGenericsPCH::run()
{
    START_BENCHMARK( "Task_MainGenericsPCH: " << m_environment.printPath( m_projectTree.getGenericsPCH() ) );
        
    VERIFY_RTE( m_session_interface );
    
    const eg::interface::Root* pInterfaceRoot = m_session_interface->getTreeRoot();
    
    //generate the generics code
    {
        std::ostringstream osImpl;
        eg::generateGenericsHeader( osImpl, *m_session_interface );
        boost::filesystem::updateFileIfChanged( m_projectTree.getGenericsHeader(), osImpl.str() );
    }
    
    build::Compilation cmd( m_projectTree.getGenericsHeader(), 
                            m_projectTree.getGenericsPCH(), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getInterfacePCH() );
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_ComponentIncludePCH::run()
{
    START_BENCHMARK( "Task_ComponentIncludePCH: " << m_environment.printPath( m_projectTree.getComponentIncludePCH( m_component ) ) );
    
    {
        std::ostringstream os;
        for( const boost::filesystem::path& includePath : m_projectTree.getComponentIncludeFiles( m_environment, m_component ) )
        {
            os << "#include \"" << includePath.string() << "\"\n";
        }    
        boost::filesystem::updateFileIfChanged( m_projectTree.getComponentIncludeHeader( m_component ), os.str() );
    }
    
    build::Compilation cmd( m_projectTree.getComponentIncludeHeader( m_component ), 
                            m_projectTree.getComponentIncludePCH( m_component ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.includeDirs = m_projectTree.getComponentIncludeDirectories( m_environment, m_component );
        cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_ComponentInterfacePCH::run()
{
    START_BENCHMARK( "Task_ComponentInterfacePCH: " << m_environment.printPath( m_projectTree.getComponentInterfacePCH( m_component ) ) );
    
    build::Compilation cmd( m_projectTree.getInterfaceHeader(), 
                            m_projectTree.getComponentInterfacePCH( m_component ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.includeDirs = m_projectTree.getComponentIncludeDirectories( m_environment, m_component );
        cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getComponentIncludePCH( m_component ) );
        cmd.egdb = m_projectTree.getComponentParserDatabase( m_component );
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_ComponentGenericsPCH::run()
{
    START_BENCHMARK( "Task_ComponentGenericsPCH: " << m_environment.printPath( m_projectTree.getComponentGenericsPCH( m_component ) ) );
    
    build::Compilation cmd( m_projectTree.getGenericsHeader(), 
                            m_projectTree.getComponentGenericsPCH( m_component ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.includeDirs = m_projectTree.getComponentIncludeDirectories( m_environment, m_component );
        cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getComponentIncludePCH( m_component ) );
        cmd.inputPCH.push_back( m_projectTree.getComponentInterfacePCH( m_component ) );
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_OperationsHeader::run()
{
    START_BENCHMARK( "Task_OperationsHeader: " << m_environment.printPath( m_sourceFile ) );
    
    VERIFY_RTE( m_session_interface );
    
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    
    VERIFY_RTE( pTranslationUnit );
    
    {
        std::ostringstream osOperations;
        eg::generateOperationSource( osOperations, m_session_interface->getTreeRoot(), *pTranslationUnit );
        boost::filesystem::updateFileIfChanged( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ), osOperations.str() );
    }
}

void Task_OperationsPublicPCH::run()
{
    VERIFY_RTE( m_session_interface );
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    VERIFY_RTE( pTranslationUnit );
    const std::string strTUName = pTranslationUnit->getName();
    
    START_BENCHMARK( "Task_OperationsPublicPCH: " << m_environment.printPath( m_projectTree.getOperationsPublicPCH( strTUName ) ) );
    
    build::Compilation cmd( m_projectTree.getOperationsHeader( strTUName ), 
                            m_projectTree.getOperationsPublicPCH( strTUName ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getInterfacePCH() );
        cmd.inputPCH.push_back( m_projectTree.getGenericsPCH() );
        cmd.egdb = m_projectTree.getInterfaceDatabaseFile();
        cmd.egtu = m_projectTree.getTUDBName( strTUName );
        cmd.egtuid = pTranslationUnit->getDatabaseFileID();
    }
    invokeCachedCompiler( m_environment, cmd );
}

void Task_OperationsPrivatePCH::run()
{
    VERIFY_RTE( m_session_interface );
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    VERIFY_RTE( pTranslationUnit );
    const std::string strTUName = pTranslationUnit->getName();
    
    START_BENCHMARK( "Task_OperationsPrivatePCH: " << m_environment.printPath( m_projectTree.getOperationsPrivatePCH( strTUName ) ) );
    
    build::Compilation cmd( m_projectTree.getOperationsHeader( strTUName ), 
                            m_projectTree.getOperationsPrivatePCH( strTUName ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getComponentIncludePCH( m_component ) );
        cmd.inputPCH.push_back( m_projectTree.getComponentInterfacePCH( m_component ) );
        cmd.inputPCH.push_back( m_projectTree.getComponentGenericsPCH( m_component ) );
        cmd.egdb = m_projectTree.getInterfaceDatabaseFile();
        cmd.egtu = m_projectTree.getTUDBName( strTUName );
        cmd.egtuid = pTranslationUnit->getDatabaseFileID();
        cmd.defines.push_back( pTranslationUnit->getCHD().getHostDefine() );
    }
    invokeCachedCompiler( m_environment, cmd );    
}

void Task_ImplementationSession::run()
{
    START_BENCHMARK( "Task_ImplementationSession: " );
    
    //use the interface session to determine the files
    eg::IndexedFile::FileIDtoPathMap allFiles;
    {
        allFiles.insert( std::make_pair( eg::IndexedObject::MASTER_FILE, m_projectTree.getInterfaceDatabaseFile() ) );
        
        const eg::TranslationUnitAnalysis& translationUnits =
            m_session_interface->getTranslationUnitAnalysis();
        
        //load all the translation unit analysis files
        for( const eg::TranslationUnit* pTranslationUnit : translationUnits.getTranslationUnits() )
        {
            if( boost::filesystem::exists( m_projectTree.getTUDBName( pTranslationUnit->getName() ) ) )
            {
                allFiles.insert( std::make_pair( 
                    pTranslationUnit->getDatabaseFileID(), 
                    m_projectTree.getTUDBName( pTranslationUnit->getName() ) ) );
            }
        }
    }
    
    {
        //perform the full program analysis
        m_session_implementation = std::make_unique< eg::ImplementationSession >( allFiles );
        m_session_implementation->fullProgramAnalysis();
        m_session_implementation->store( m_projectTree.getAnalysisFileName() );
    }
}


void build_interface( const boost::filesystem::path& projectDirectory, const std::string& strProject, const std::string& strCompilationFlags )
{
    Environment environment;

    START_BENCHMARK( "Total time compiling megastructure interface for: " << strProject );
    
    ProjectTree projectTree( environment, projectDirectory, strProject );
    
    BuildState buildState( environment, projectTree, m_config, strCompilationFlags );
    
    build::Task::PtrVector tasks;
    {
        Task_ParserSession* pParserSession = new Task_ParserSession( buildState );
        tasks.push_back( build::Task::Ptr( pParserSession ) );
        
        using ComponentTaskMap = std::map< Component, Task_ParserSessionCopy* >;
        ComponentTaskMap componentDBCopies;
        build::Task::RawPtrSet parserSessionCopies;
        for( Coordinator::Ptr pCoordinator : projectTree.getCoordinators() )
        {
            const std::string& strCoordinatorName = pCoordinator->name();
            
            for( HostName::Ptr pHostName : pCoordinator->getHostNames() )
            {
                const std::string& strHostName = pHostName->name();
                for( ProjectName::Ptr pProjectName : pHostName->getProjectNames() )
                {
                    if( pProjectName->name() == strProject )
                    {
                        Component component = { pCoordinator, pHostName, pProjectName };
                        Task_ParserSessionCopy* pParserSessionCopy = new Task_ParserSessionCopy( buildState, pParserSession, component );
                        tasks.push_back( build::Task::Ptr( pParserSessionCopy ) );
                        parserSessionCopies.insert( pParserSessionCopy );
                        componentDBCopies.insert( std::make_pair( component, pParserSessionCopy ) );
                    }
                }
            }
        }
        
        Task_MainIncludePCH*   pMainIncludePCH     = new Task_MainIncludePCH( buildState, pParserSession );
        Task_MainInterfacePCH* pMainInterfacePCH   = new Task_MainInterfacePCH( buildState, pMainIncludePCH );
        Task_InterfaceSession* pInterfaceSession   = new Task_InterfaceSession( buildState, pMainInterfacePCH, parserSessionCopies );
        Task_MainGenericsPCH*  pMainGenericsPCH    = new Task_MainGenericsPCH( buildState, pInterfaceSession );
        
        tasks.push_back( build::Task::Ptr( pMainIncludePCH     ) );
        tasks.push_back( build::Task::Ptr( pMainInterfacePCH   ) );
        tasks.push_back( build::Task::Ptr( pInterfaceSession   ) );
        tasks.push_back( build::Task::Ptr( pMainGenericsPCH    ) );
        
        build::Task::RawPtrSet publicOperationsTasks;
        publicOperationsTasks.insert( pMainGenericsPCH );
        
        for( Coordinator::Ptr pCoordinator : projectTree.getCoordinators() )
        {
            const std::string& strCoordinatorName = pCoordinator->name();
            
            for( HostName::Ptr pHostName : pCoordinator->getHostNames() )
            {
                const std::string& strHostName = pHostName->name();
                for( ProjectName::Ptr pProjectName : pHostName->getProjectNames() )
                {
                    if( pProjectName->name() == strProject )
                    {
                        Component component = { pCoordinator, pHostName, pProjectName };
                        
                        Task_ParserSessionCopy* pParserDBCopy = componentDBCopies[ component ];
                        VERIFY_RTE( pParserDBCopy );
                        
                        Task_ComponentIncludePCH* pComponentIncludePCH = 
                            new Task_ComponentIncludePCH( buildState, pMainIncludePCH, component );
                        
                        Task_ComponentInterfacePCH* pComponentInterfacePCH = 
                            new Task_ComponentInterfacePCH( buildState, pComponentIncludePCH, pParserDBCopy, component );
                        
                        Task_ComponentGenericsPCH* pComponentGenericsPCH = 
                            new Task_ComponentGenericsPCH( buildState, pComponentInterfacePCH, pInterfaceSession, component );
                            
                        tasks.push_back( build::Task::Ptr( pComponentIncludePCH ) );
                        tasks.push_back( build::Task::Ptr( pComponentInterfacePCH ) );
                        tasks.push_back( build::Task::Ptr( pComponentGenericsPCH ) );
                        
                        const std::vector< boost::filesystem::path >& sourceFiles = pProjectName->sourceFiles();
                        for( const boost::filesystem::path& sourceFile : sourceFiles )
                        {
                            Task_OperationsHeader* pOperationsHeader =
                                new Task_OperationsHeader( buildState, pInterfaceSession, sourceFile );
                            
                            Task_OperationsPublicPCH* pOperationsPublicPCH = 
                                new Task_OperationsPublicPCH( buildState, pMainGenericsPCH, pOperationsHeader, sourceFile );
                                
                            Task_OperationsPrivatePCH* pOperationsPrivatePCH = 
                                new Task_OperationsPrivatePCH( buildState, pComponentGenericsPCH, pOperationsPublicPCH, component, sourceFile );
                                
                            tasks.push_back( build::Task::Ptr( pOperationsHeader ) );
                            tasks.push_back( build::Task::Ptr( pOperationsPrivatePCH ) );
                            tasks.push_back( build::Task::Ptr( pOperationsPublicPCH ) );
                            
                            publicOperationsTasks.insert( pOperationsPublicPCH );
                        }
                    }
                }
            }
        }
        
        
        Task_ImplementationSession* pImplementationSession = new Task_ImplementationSession( buildState, publicOperationsTasks );
        tasks.push_back( build::Task::Ptr( pImplementationSession ) );
    }
    
    build::Scheduler scheduler( tasks );
    
    scheduler.run();
    
}