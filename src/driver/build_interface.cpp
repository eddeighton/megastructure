
#include "build_interface.hpp"

#include "eg_parser/parser.hpp"

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
#include <functional>

namespace build
{
namespace Interface
{

static DiagnosticsConfig m_config;

struct ParserCallbackImpl : eg::EG_PARSER_CALLBACK, eg::FunctionBodyGenerator
{
    using FunctionMap = std::map< eg::IndexedObject::Index, std::ostringstream >;
    FunctionMap m_contextMap, m_exportMap;
    
    //eg::EG_PARSER_CALLBACK
    virtual void contextBody( const eg::input::Context* pContext, const char* pszBodyText )
    {
        m_contextMap[ pContext->getIndex() ] << pszBodyText;
    }
    virtual void exportBody( const eg::input::Export* pExport, const char* pszBodyText )
    {
        m_exportMap[ pExport->getIndex() ] << pszBodyText;
    }
    
    //eg::FunctionBodyGenerator
    virtual void printFunctionBody( const eg::input::Context* pContext, std::ostream& os )
    {
        os << m_contextMap[ pContext->getIndex() ].str();
    }
    virtual void printExportBody( const eg::input::Export* pExport, std::ostream& os )
    {
        os << m_exportMap[ pExport->getIndex() ].str();
    }
};
ParserCallbackImpl m_functionBodyHandler;

void Task_ParserSession::run()
{
    //do nothing since doing this in main function
}

void Task_ParserSessionCopy::run()
{
    m_taskInfo.taskName( "ParserSessionCopy" );
    m_taskInfo.source( m_projectTree.getParserDatabaseFile() );
    m_taskInfo.target( m_projectTree.getComponentParserDatabase( m_component ) );
    updateProgress();
    
    if( m_parserChanged )
    {
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
    m_taskInfo.cached( true );
    m_taskInfo.complete( true );
}

void Task_MainIncludePCH::run()
{
    m_taskInfo.taskName( "MainIncludePCH" );
    m_taskInfo.source( m_projectTree.getIncludeHeader() );
    m_taskInfo.target( m_projectTree.getIncludePCH() );
    updateProgress();
    
    VERIFY_RTE( m_session_parser );
    
    const eg::interface::Root* pInterfaceRoot = m_session_parser->getTreeRoot();
    
    //generate the includes header
    std::size_t hashCode;
    {
        //careful to only write to the file if it has been modified
        VERIFY_RTE( pInterfaceRoot );
        std::ostringstream osInclude;
        eg::generateIncludeHeader( osInclude, 
            pInterfaceRoot, 
            m_projectTree.getSystemIncludes(), 
            m_projectTree.getUserIncludes( m_environment ) );
            
        hashCode = build::hash_strings( { osInclude.str(), m_strCompilationFlags } );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
        m_stash.setHashCode( m_projectTree.getIncludePCH(), hashCode );
            
        if( m_stash.restore( m_projectTree.getIncludePCH(), hashCode ) )
        {
            m_taskInfo.cached( true );
            m_taskInfo.complete( true );
            return;
        }
            
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
    
    invokeCompiler( m_environment, cmd );
            
    m_stash.stash( m_projectTree.getIncludePCH(), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_MainInterfacePCH::run()
{
    m_taskInfo.taskName( "MainInterfacePCH" );
    m_taskInfo.source( m_projectTree.getInterfaceHeader() );
    m_taskInfo.target( m_projectTree.getInterfacePCH() );
    updateProgress();
        
    VERIFY_RTE( m_session_parser );
    
    const eg::interface::Root* pInterfaceRoot = m_session_parser->getTreeRoot();
    
    //generate the interface
    std::ostringstream osInterface;
    eg::generateInterface( osInterface, 
        m_session_parser->getTreeRoot(), 
        m_session_parser->getIdentifiers() );
    
    std::size_t interfaceHash = build::hash_strings( { m_strCompilationFlags, osInterface.str() } );
    m_stash.setHashCode( m_projectTree.getInterfaceHeader(), interfaceHash );
    
    interfaceHash = build::hash_combine( interfaceHash, 
        m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    interfaceHash = build::hash_combine( interfaceHash, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    m_stash.setHashCode( m_projectTree.getInterfacePCH(), interfaceHash );
    
    if( m_stash.restore( m_projectTree.getInterfacePCH(), interfaceHash ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
    boost::filesystem::updateFileIfChanged( m_projectTree.getInterfaceHeader(), osInterface.str() );
    
    build::Compilation cmd( m_projectTree.getInterfaceHeader(), 
                            m_projectTree.getInterfacePCH(), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputPCH{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.egdb = m_projectTree.getParserDatabaseFile();
    }
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getInterfacePCH(), interfaceHash );
    
    m_taskInfo.complete( true );
}

void Task_InterfaceSession::run()
{
    m_taskInfo.taskName( "InterfaceSession" );
    m_taskInfo.source( m_projectTree.getParserDatabaseFile() );
    m_taskInfo.target( m_projectTree.getInterfaceDatabaseFile() );
    updateProgress();
    
    VERIFY_RTE( m_session_parser );
    
    std::size_t interfaceDBHash = build::hash_strings( { m_strCompilationFlags } );
    interfaceDBHash = build::hash_combine( interfaceDBHash, 
        m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    interfaceDBHash = build::hash_combine( interfaceDBHash, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    interfaceDBHash = build::hash_combine( interfaceDBHash, m_stash.getHashCode( m_projectTree.getInterfacePCH() ) );
    m_stash.setHashCode( m_projectTree.getInterfaceDatabaseFile(), interfaceDBHash );
    
    if( m_stash.restore( m_projectTree.getInterfaceDatabaseFile(), interfaceDBHash ) )
    {
        m_session_interface = std::make_unique< eg::InterfaceSession >( 
            m_projectTree.getInterfaceDatabaseFile() );
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
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
    
    m_session_interface->store( m_projectTree.getInterfaceDatabaseFile() );
    m_stash.stash( m_projectTree.getInterfaceDatabaseFile(), interfaceDBHash );
    
    m_taskInfo.complete( true );
}

void Task_MainGenericsPCH::run()
{
    m_taskInfo.taskName( "MainGenericsPCH" );
    m_taskInfo.source( m_projectTree.getGenericsHeader() );
    m_taskInfo.target( m_projectTree.getGenericsPCH() );
    updateProgress();
        
    VERIFY_RTE( m_session_interface );
    
    const eg::interface::Root* pInterfaceRoot = m_session_interface->getTreeRoot();
    
    //generate the generics code
    std::size_t hashCode;
    {
        std::ostringstream osImpl;
        eg::generateGenericsHeader( osImpl, *m_session_interface );
        
        hashCode = build::hash_strings( { osImpl.str(), m_strCompilationFlags } );
        m_stash.setHashCode( m_projectTree.getGenericsHeader(), hashCode );
        
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getInterfacePCH() ) );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getInterfaceDatabaseFile() ) );
        m_stash.setHashCode( m_projectTree.getGenericsPCH(), hashCode );
        
        if( m_stash.restore( m_projectTree.getGenericsPCH(), hashCode ) )
        {
            m_taskInfo.cached( true );
            m_taskInfo.complete( true );
            return;
        }
        
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
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getGenericsPCH(), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_ComponentIncludePCH::run()
{
    m_taskInfo.taskName( "ComponentIncludePCH" );
    m_taskInfo.source( m_projectTree.getComponentIncludeHeader( m_component ) );
    m_taskInfo.target( m_projectTree.getComponentIncludePCH( m_component ) );
    updateProgress();
    
    std::size_t hashCode;
    {
        std::ostringstream os;
        for( const boost::filesystem::path& includePath : m_projectTree.getComponentIncludeFiles( m_environment, m_component ) )
        {
            os << "#include \"" << includePath.string() << "\"\n";
        }

        hashCode = build::hash_strings( { os.str(), m_strCompilationFlags } );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
        hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
        m_stash.setHashCode( m_projectTree.getComponentIncludePCH( m_component ), hashCode );
            
        if( m_stash.restore( m_projectTree.getComponentIncludePCH( m_component ), hashCode ) )
        {
            m_taskInfo.cached( true );
            m_taskInfo.complete( true );
            return;
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
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getComponentIncludePCH( m_component ), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_ComponentInterfacePCH::run()
{
    m_taskInfo.taskName( "ComponentInterfacePCH" );
    m_taskInfo.source( m_projectTree.getInterfaceHeader() );
    m_taskInfo.target( m_projectTree.getComponentInterfacePCH( m_component ) );
    updateProgress();
    
    std::size_t hashCode = m_stash.getHashCode( m_projectTree.getInterfaceHeader() );
    hashCode = build::hash_combine( hashCode, build::hash_strings( { m_strCompilationFlags } ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentIncludePCH( m_component ) ) );
    m_stash.setHashCode( m_projectTree.getComponentInterfacePCH( m_component ), hashCode );
    
    if( m_stash.restore( m_projectTree.getComponentInterfacePCH( m_component ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
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
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getComponentInterfacePCH( m_component ), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_ComponentGenericsPCH::run()
{
    m_taskInfo.taskName( "ComponentGenericsPCH" );
    m_taskInfo.source( m_projectTree.getGenericsHeader() );
    m_taskInfo.target( m_projectTree.getComponentGenericsPCH( m_component ) );
    updateProgress();
    
    std::size_t hashCode = m_stash.getHashCode( m_projectTree.getGenericsHeader() );
    hashCode = build::hash_combine( hashCode, build::hash_strings( { m_strCompilationFlags } ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentIncludePCH( m_component ) ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentInterfacePCH( m_component ) ) );
    m_stash.setHashCode( m_projectTree.getComponentGenericsPCH( m_component ), hashCode );
    
    if( m_stash.restore( m_projectTree.getComponentGenericsPCH( m_component ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
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
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getComponentGenericsPCH( m_component ), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_OperationsHeader::run()
{
    VERIFY_RTE( m_session_interface );
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    VERIFY_RTE( pTranslationUnit );
    
    m_taskInfo.taskName( "OperationsHeader" );
    m_taskInfo.source( std::string{"none"} );
    m_taskInfo.target( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ) );
    updateProgress();
    
    std::ostringstream osOperations;
    eg::generateOperationSource( osOperations, m_session_interface->getTreeRoot(), *pTranslationUnit, m_functionBodyHandler );
    
    std::size_t hashCode = build::hash_strings( { m_strCompilationFlags, osOperations.str() } );
    m_stash.setHashCode( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ), hashCode );
        
    boost::filesystem::updateFileIfChanged( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ), osOperations.str() );
    
    m_taskInfo.cached( true );
    m_taskInfo.complete( true );
}

void Task_OperationsPublicPCH::run()
{
    VERIFY_RTE( m_session_interface );
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    VERIFY_RTE( pTranslationUnit );
    const std::string strTUName = pTranslationUnit->getName();
    
    m_taskInfo.taskName( "OperationsPublicPCH" );
    m_taskInfo.source( m_projectTree.getOperationsHeader( strTUName ) );
    m_taskInfo.target( m_projectTree.getOperationsPublicPCH( strTUName ) );
    updateProgress();
    
    std::size_t hashCode = m_stash.getHashCode( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ) );
    hashCode = build::hash_combine( hashCode, build::hash_strings( { m_strCompilationFlags } ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getInterfacePCH() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getGenericsPCH() ) );
    m_stash.setHashCode( m_projectTree.getOperationsPublicPCH( strTUName ), hashCode );
    
    if( m_stash.restore( m_projectTree.getOperationsPublicPCH( strTUName ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
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
    invokeCompiler( m_environment, cmd );
    
    m_stash.stash( m_projectTree.getOperationsPublicPCH( strTUName ), hashCode );
    
    m_taskInfo.complete( true );
}

void Task_OperationsPrivatePCH::run()
{
    VERIFY_RTE( m_session_interface );
    const eg::TranslationUnitAnalysis& tuAnalysis = m_session_interface->getTranslationUnitAnalysis();
    const eg::TranslationUnit* pTranslationUnit = tuAnalysis.getTU( m_sourceFile );
    VERIFY_RTE( pTranslationUnit );
    const std::string strTUName = pTranslationUnit->getName();
    
    m_taskInfo.taskName( "OperationsPrivatePCH" );
    m_taskInfo.source( m_projectTree.getOperationsHeader( strTUName ) );
    m_taskInfo.target( m_projectTree.getOperationsPrivatePCH( strTUName ) );
    updateProgress();
    
    std::size_t hashCode = m_stash.getHashCode( m_projectTree.getOperationsHeader( pTranslationUnit->getName() ) );
    hashCode = build::hash_combine( hashCode, build::hash_strings( { m_strCompilationFlags } ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentIncludePCH( m_component ) ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentInterfacePCH( m_component ) ) );
    hashCode = build::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentGenericsPCH( m_component ) ) );
    m_stash.setHashCode( m_projectTree.getOperationsPrivatePCH( strTUName ), hashCode );
    
    if( m_stash.restore( m_projectTree.getOperationsPrivatePCH( strTUName ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
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
    invokeCompiler( m_environment, cmd );  
    
    m_stash.stash( m_projectTree.getOperationsPrivatePCH( strTUName ), hashCode );  
    
    m_taskInfo.complete( true );
}

void Task_ImplementationSession::run()
{    
    m_taskInfo.taskName( "ImplementationSession" );
    m_taskInfo.source( m_projectTree.getInterfaceDatabaseFile() );
    m_taskInfo.target( m_projectTree.getAnalysisFileName() );
    updateProgress();
    
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
    
    ProjectTree projectTree( environment, projectDirectory, strProject );
    
    build::Stash stash( environment, projectTree.getStashFolder() );
    
    BuildState buildState( environment, projectTree, m_config, strCompilationFlags, stash );
    
    //parse the input source code generating the parser.db
    {
        eg::ParserSession::SourceCodeTree sourceCodeTree;
        {
            sourceCodeTree.root = projectTree.getSourceFolder();
            projectTree.getSourceFilesMap( sourceCodeTree.files );
        }
        
        {
            buildState.m_session_parser = 
                std::make_unique< eg::ParserSession >( 
                    &m_functionBodyHandler,
                    environment.getParserDll(),
                    projectTree.getSourceFolder(), std::cout );
            buildState.m_session_parser->parse( sourceCodeTree );
            buildState.m_session_parser->buildAbstractTree();
            buildState.m_session_parser->store( projectTree.getParserDatabaseFile() );
            
            std::size_t parserHash = build::hash_file( projectTree.getParserDatabaseFile() );
            parserHash = build::hash_combine( parserHash, build::hash_strings( { strCompilationFlags } ) );
            stash.setHashCode( projectTree.getParserDatabaseFilePreInterfaceAnalysis(), parserHash );
            
            if( stash.restore( projectTree.getParserDatabaseFilePreInterfaceAnalysis(), parserHash ) )
            {
                //if( boost::filesystem::exists( projectTree.getParserDatabaseFile() ) )
                //{
                //    boost::filesystem::remove( projectTree.getParserDatabaseFile() );
                //}
                buildState.m_parserChanged = false;
            }
            else
            {
                buildState.m_parserChanged = true;
                if( boost::filesystem::exists( projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) )
                {
                    boost::filesystem::remove( projectTree.getParserDatabaseFilePreInterfaceAnalysis() );
                }
                boost::filesystem::copy( projectTree.getParserDatabaseFile(), 
                    projectTree.getParserDatabaseFilePreInterfaceAnalysis() );
                stash.stash( projectTree.getParserDatabaseFilePreInterfaceAnalysis(), parserHash );
            }
        }
    }
    
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
                            new Task_ComponentInterfacePCH( buildState, pComponentIncludePCH, pParserDBCopy, pMainInterfacePCH, component );
                        
                        Task_ComponentGenericsPCH* pComponentGenericsPCH = 
                            new Task_ComponentGenericsPCH( buildState, pComponentInterfacePCH, pInterfaceSession, pMainGenericsPCH, component );
                            
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
    
    stash.saveHashCodes( projectTree.getBuildInfoFile() );
    
}

}//namespace build
}//namespace Interface
