

#ifndef EG_PROJECT_UTIL_10_06_2019
#define EG_PROJECT_UTIL_10_06_2019

#include "megaxml/mega_schema.hxx"

#include "environment_string.hpp"

#include <boost/filesystem.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#include <boost/process/env.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#pragma warning( pop )

#include <memory>
#include <vector>
#include <optional>
#include <map>
#include <functional>

class XMLManager
{
public:
    using XMLDocPtr     = std::shared_ptr< megaxml::EG >;
    using XMLDocPtrMap  = std::map< boost::filesystem::path, XMLDocPtr >;
    
    static XMLDocPtr load( const boost::filesystem::path& docPath );
    
private:
    static XMLDocPtrMap m_documents;
};

class Environment
{
public:
    static const boost::filesystem::path EG_FILE_EXTENSION;
    static const boost::filesystem::path INSTALLATION_FILE;
    static const boost::filesystem::path WIZARD_FILE;
    static const boost::filesystem::path PYTHON_FILE_EXTENSION;
    static const std::string ENV_KEY_EG_INSTALLATION;
    static const std::string ENV_KEY_CURRENT_PROJECT;
    static const std::string ENV_KEY_LOG_FOLDER;
    //static const std::string ENV_KEY_WINDOWS_10_SDK;
    //static const std::string ENV_KEY_VISUALSTUDIO;
    //static const std::string ENV_KEY_PYTHONHOME;
    
    void commonCtor();
    
    Environment();
    Environment( const boost::filesystem::path& projectDir );
    Environment( const boost::process::environment& env, const boost::filesystem::path& projectDir );
    
    std::string printPath( const boost::filesystem::path& thePath ) const;
    void startCompilationCommand( std::ostream& os ) const;
    void startLogCommand( std::ostream& os ) const;
    void startDriverCommand( std::ostream& os ) const;
    const boost::filesystem::path& getEGLibraryInclude() const;
    const boost::filesystem::path& getClangPluginDll() const;
    const boost::filesystem::path& getLogFolderPath() const;
    
    std::string expand( const std::string& strPath ) const;
    
    const std::string get( const std::string& strKey ) const;
    
    const megaxml::Host& getHost( const std::string& strHost ) const;
    const megaxml::Package& getPackage( const std::string& strPackage ) const;
    
    const boost::process::environment& getEnvironment() const { return m_environment; }
private:
    boost::process::environment m_environment;
};

class Project
{
public:
    Project( const boost::filesystem::path& projectDir, 
        const Environment& environment, const megaxml::Project& project );
    Project( const boost::filesystem::path& projectDir, 
        const Environment& environment, const megaxml::Project& project, const std::string& strBuildCommand );
    
    const boost::filesystem::path& getProjectDir() const { return m_projectDir; }
    const megaxml::Project& getProject() const { return m_project; }
	const megaxml::Host& getHost() const { return m_host; }
    
    std::vector< boost::filesystem::path > getHostIncludes() const;
private:
    const boost::filesystem::path m_projectDir;
    Environment m_environment;
    const megaxml::Project& m_project;
    const megaxml::Host& m_host;
};

#endif //EG_PROJECT_UTIL_10_06_2019