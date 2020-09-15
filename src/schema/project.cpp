

#include "project.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "common/assert_verify.hpp"
#include "common/file.hpp"
#include "common/escape.hpp"

#include <sstream>
#include <set>
#include <iomanip>
#include <iostream>

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

XMLManager::XMLDocPtrMap XMLManager::m_documents;

XMLManager::XMLDocPtr XMLManager::load( const boost::filesystem::path& docPath )
{
    XMLDocPtrMap::const_iterator iFind = m_documents.find( docPath );
    if( iFind != m_documents.end() )
    {
        return iFind->second;
    }
    else
    {
        try
        {
            megaxml::EG_paggr eg_p;
            xml_schema::document_pimpl doc_p( eg_p.root_parser(), eg_p.root_name() );
             
            eg_p.pre();
            doc_p.parse( docPath.string() );
            
            XMLDocPtr pDocument( eg_p.post() );
            
            m_documents.insert( std::make_pair( docPath, pDocument ) );
        
            return pDocument;
        }
        catch( const xml_schema::parser_exception& e )
        {
            std::ostringstream os;
            os << "XML error opening: " << docPath.generic_string() << " " << e.line() << ":" << e.column() << ": " << e.text();
            throw std::runtime_error( os.str() );
        }
    }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
const boost::filesystem::path Environment::EG_FILE_EXTENSION = std::string( ".eg" );
const boost::filesystem::path Environment::INSTALLATION_FILE = std::string( "installation.xml" );
const boost::filesystem::path Environment::WIZARD_FILE = std::string( "wizard.xml" );

const boost::filesystem::path Environment::PYTHON_FILE_EXTENSION = std::string( ".py" );

const std::string Environment::ENV_KEY_EG_INSTALLATION = "EG";
const std::string Environment::ENV_KEY_LOG_FOLDER = "MEGALOG";

/*
const std::string Environment::ENV_KEY_WINDOWS_10_SDK = "WINDOWS_10_SDK";
const std::string Environment::ENV_KEY_VISUALSTUDIO = "EG_VISUAL_STUDIO";
const std::string Environment::ENV_KEY_PYTHONHOME = "PYTHONHOME";*/

const std::string Environment::ENV_KEY_CURRENT_PROJECT = "PROJECT";

void Environment::commonCtor()
{
	if( m_environment.count( ENV_KEY_EG_INSTALLATION ) == 0U )
	{
        THROW_RTE( "Missing EG environment variable for eg installation location." );
	}
	
    /*std::optional< boost::filesystem::path > egInstallationPath;
    std::optional< boost::filesystem::path > egInstallationXMLFile;
    {
        boost::system::error_code errorCode;
        boost::filesystem::path currentProgramLoc = boost::dll::program_location( errorCode );
        VERIFY_RTE_MSG( !errorCode, "Error attempting to locate program location: " << errorCode );
        boost::filesystem::path pathIter = currentProgramLoc.parent_path();
        while( !pathIter.empty() )
        {
            boost::filesystem::path nextPath = pathIter / "installation.xml";
            if( boost::filesystem::exists( nextPath ) && boost::filesystem::is_regular_file( nextPath ) )
            {
                egInstallationPath = pathIter;
                egInstallationXMLFile = nextPath;
                break;
            }
            else
            {
                pathIter = pathIter.parent_path();
            }
        }
    }
    if( !egInstallationPath || !egInstallationXMLFile )
    {
        THROW_RTE( "Failed to locate eg installation.  Missing " );
    }
    else
    {
        m_environment[ ENV_KEY_EG_INSTALLATION ] = egInstallationPath.value().generic_string();
        
        m_environment[ "PATH" ] += egInstallationPath.value().generic_string();
    }
    
    //std::cout << "loading installation: " << egInstallationXMLFile.value().generic_string() << std::endl;
    //m_environment[ ENV_KEY_WINDOWS_10_SDK ] = "C:/Program Files (x86)/Windows Kits/10";
    
    try
    {
        installationxml::Installation_paggr installation_p;
        xml_schema::document_pimpl doc_p( installation_p.root_parser(), installation_p.root_name() );
        installation_p.pre();
        doc_p.parse( egInstallationXMLFile.value().generic_string() );
        installationxml::Installation installation = installation_p.post();
        
        if( m_environment.count( ENV_KEY_WINDOWS_10_SDK ) == 0U )
        {
            boost::filesystem::path xmlPath = installation.SDK();
            if( boost::filesystem::exists( xmlPath ) )
            {
                m_environment[ ENV_KEY_WINDOWS_10_SDK ] = xmlPath.generic_string();
            }
            else
            {
                THROW_RTE( "Failed to locate Windows 10 SDK Installation" );
            }
        }
        
        if( m_environment.count( ENV_KEY_PYTHONHOME ) == 0U )
        {
            boost::filesystem::path xmlPath = installation.Python();
            if( boost::filesystem::exists( xmlPath ) )
            {
                m_environment[ ENV_KEY_PYTHONHOME ] = xmlPath.generic_string();
            }
            else
            {
                THROW_RTE( "Failed to locate Python3 Installation" );
            }
        }
        
        if( m_environment.count( ENV_KEY_VISUALSTUDIO ) == 0U )
        {
            boost::filesystem::path xmlPath = installation.Toolchain();
            if( boost::filesystem::exists( xmlPath ) )
            {
                m_environment[ ENV_KEY_VISUALSTUDIO ] = xmlPath.generic_string();
            }
            else
            {
                THROW_RTE( "Failed to locate Visual Studio 2017 Installation" );
            }
        }
    }
    catch( const xml_schema::parser_exception& e )
    {
        std::ostringstream os;
        os << "XML error opening: " << egInstallationXMLFile.value().generic_string() << 
            " " << e.line() << ":" << e.column() << ": " << e.text();
        throw std::runtime_error( os.str() );
    }*/
}

Environment::Environment()
    :   m_environment( boost::this_process::environment() )
{
    commonCtor();
}

Environment::Environment( const boost::filesystem::path& projectDir )
    :   m_environment( boost::this_process::environment() )
{
    m_environment[ ENV_KEY_CURRENT_PROJECT ] = projectDir.generic_string();
    commonCtor();
}

Environment::Environment( const boost::process::environment& env, const boost::filesystem::path& projectDir )
    :   m_environment( env )
{
    m_environment[ ENV_KEY_CURRENT_PROJECT ] = projectDir.generic_string();
    commonCtor();
}

const std::string Environment::get( const std::string& strKey ) const
{
    boost::process::environment::const_iterator iFind = m_environment.find( strKey );
    VERIFY_RTE_MSG( iFind != m_environment.end(), "Failed to locate environment variable: " << strKey );
    std::vector< std::string > strs = iFind->to_vector();
    VERIFY_RTE_MSG( !strs.empty(), "Environment variable is empty: " << strKey );
    std::ostringstream os;
    std::copy( strs.begin(), strs.end(), std::ostream_iterator< std::string >( os ) );
    return os.str();
}
    
std::string Environment::printPath( const boost::filesystem::path& thePath ) const
{
    return Common::escapeString( thePath.generic_string() );
}

void Environment::startCompilationCommand( std::ostream& os ) const
{
    static boost::filesystem::path CLANG = boost::process::search_path( "clang.exe" );
    if( CLANG.empty() )
    {
		THROW_RTE("Failed to locate clang.exe in path.  Please ensure the correct clang.exe is configured.");
        /*boost::process::environment::const_iterator iFind = m_environment.find( "EG_CLANG" );
        if( iFind != m_environment.end() )
        {
            std::vector< std::string > strs = iFind->to_vector();
            std::ostringstream os;
            std::copy( strs.begin(), strs.end(), std::ostream_iterator< std::string >( os ) );
            CLANG = os.str();
        }
        if( CLANG.empty() )
            CLANG = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "third_party/install/llvm/bin/clang.exe";*/
    }
    os << printPath( CLANG ) << " ";
}


void Environment::startLogCommand( std::ostream& os ) const
{
    static boost::filesystem::path EGLOG;
    if( EGLOG.empty() )
    {
        EGLOG = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eglog.exe";
    }
    os << printPath( EGLOG ) << " ";
}

void Environment::startDriverCommand( std::ostream& os ) const
{
    static boost::filesystem::path EGDRIVER;
    if( EGDRIVER.empty() )
    {
        EGDRIVER = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eg.exe";
    }
    os << printPath( EGDRIVER ) << " ";
}

const boost::filesystem::path& Environment::getClangPluginDll() const
{
    static boost::filesystem::path EG_CLANG_PLUGIN;
    if( EG_CLANG_PLUGIN.empty() )
	{
//#ifdef _DEBUG
        //EG_CLANG_PLUGIN = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eg_clang_plugind.dll";
//#else
        EG_CLANG_PLUGIN = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eg_clang_plugin.dll";
//#endif
    }
    return EG_CLANG_PLUGIN;
}

const boost::filesystem::path& Environment::getParserDll() const
{
    static boost::filesystem::path PARSER_PLUGIN;
    if( PARSER_PLUGIN.empty() )
    {
//#ifdef _DEBUG
//        PARSER_PLUGIN = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eg_parserd.dll";
//#else
        PARSER_PLUGIN = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "bin/eg_parser.dll";
//#endif
    }
    return PARSER_PLUGIN;
}

const boost::filesystem::path& Environment::getEGLibraryInclude() const
{
    static boost::filesystem::path EG_LIBRARY;
    if( EG_LIBRARY.empty() )
    {
        EG_LIBRARY = boost::filesystem::path( get( ENV_KEY_EG_INSTALLATION ) ) / "include";
    }
    return EG_LIBRARY;
}

std::string Environment::expand( const std::string& strPath ) const
{
    return ::expand( strPath, [ this ]( const std::string& strKey ){ return get( strKey ); } );
}

const boost::filesystem::path& Environment::getLogFolderPath() const
{
    static boost::filesystem::path LOG_FOLDER_PATH;
    if( LOG_FOLDER_PATH.empty() )
    {
        LOG_FOLDER_PATH = boost::filesystem::path( get( ENV_KEY_LOG_FOLDER ) );
    }
    return LOG_FOLDER_PATH;
}

const megaxml::Host& Environment::getHost( const std::string& strHost ) const
{
    VERIFY_RTE_MSG( !strHost.empty(), "Empty host specification" );
	
	const boost::filesystem::path hostPath =
		boost::filesystem::edsCannonicalise(
			expand( strHost ) / Environment::EG_FILE_EXTENSION );

    //boost::filesystem::path hostPath = strHost / Environment::EG_FILE_EXTENSION;
    VERIFY_RTE_MSG( boost::filesystem::exists( hostPath ), "Failed to locate host at: " << hostPath.generic_string() );
    
    XMLManager::XMLDocPtr pDoc = XMLManager::load( hostPath );
    VERIFY_RTE_MSG( pDoc->choice_arm() == megaxml::EG::Host_tag, "Directory is not a host: " << hostPath.generic_string() );
    return pDoc->Host();
}

const megaxml::Package& Environment::getPackage( const std::string& strPackage ) const
{
    VERIFY_RTE_MSG( !strPackage.empty(), "Empty package specification" );
	
	const boost::filesystem::path packagePath =
		boost::filesystem::edsCannonicalise(
			expand( strPackage ) / Environment::EG_FILE_EXTENSION );
			
    //boost::filesystem::path packagePath = strPackage / Environment::EG_FILE_EXTENSION;
    VERIFY_RTE_MSG( boost::filesystem::exists( packagePath ), "Failed to locate package at: " << packagePath.generic_string() );
    
    XMLManager::XMLDocPtr pDoc = XMLManager::load( packagePath );
    VERIFY_RTE_MSG( pDoc->choice_arm() == megaxml::EG::Package_tag, "Directory is not a package: " << packagePath.generic_string() );
    return pDoc->Package();
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
Project::Project( const boost::filesystem::path& projectDir, 
    const Environment& environment, const megaxml::Project& project )
    :   m_projectDir( projectDir ),
        m_environment( environment.getEnvironment(), projectDir ),
        m_project( project ),
        m_host( m_environment.getHost( m_project.Host() ) )
{
}
        
Project::Project( const boost::filesystem::path& projectDir, 
            const Environment& environment, const megaxml::Project& project, const std::string& strBuildCommand )
    :   m_projectDir( projectDir ),
        m_environment( environment.getEnvironment(), projectDir ),
        m_project( project ),
        m_host( m_environment.getHost( m_project.Host() ) )
{
}


static void collateUserIncludeFiles( 
    const Environment& environment,
    std::set< boost::filesystem::path >& uniquified, 
    std::vector< boost::filesystem::path >& directories,
    const megaxml::Files& xmlFiles )
{
    for( const std::string& strFile : xmlFiles.Include() )
    {
        const boost::filesystem::path absPath =
            boost::filesystem::edsCannonicalise(
                environment.expand( strFile ) );
                
        if( 0 == uniquified.count( absPath ) )
        {
            uniquified.insert( absPath );
            directories.push_back( absPath );
        }
    }
}

std::vector< boost::filesystem::path > Project::getHostIncludes() const
{
    std::set< boost::filesystem::path > uniquified;
    std::vector< boost::filesystem::path > includes;
    
    if( m_host.Files_present() )
        collateUserIncludeFiles( m_environment, uniquified, includes, m_host.Files() );
    
    //for( const megaxml::Package& package : m_packages )
    //{
    //    if( package.Files_present() )
    //        collateUserIncludeFiles( m_environment, uniquified, includes, package.Files() );
    //}
    
    return includes;
}