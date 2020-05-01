
#include "program.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/dll/import.hpp> // for import_alias


namespace megastructure
{
	
std::string getComponentName( const std::string& strCoordinator, const std::string& strHostName, const std::string& strProjectName )
{
	std::ostringstream os;
	os << strCoordinator << '_' << strHostName << '_' << strProjectName << ".dll";
	return os.str();
}

boost::filesystem::path getBinFolderForProject( const boost::filesystem::path& workspacePath, const std::string& strProjectName )
{
	//return boost::filesystem::edsCannonicalise(
	//			boost::filesystem::absolute( workspacePath / "build" / strProjectName ) );
	return boost::filesystem::edsCannonicalise(
				boost::filesystem::absolute( workspacePath / "install" / strProjectName / "bin" ) );
}
    
Program::Program( Component& component, const std::string& strHostName, const std::string& strProjectName )
    :   m_component( component ),
        m_strHostName( strHostName ),
        m_strProjectName( strProjectName )
{
    const boost::filesystem::path binDirectory = getBinFolderForProject( m_component.getSlaveWorkspacePath(), m_strProjectName );
    m_strComponentName = getComponentName( m_component.getSlaveName(), m_strHostName, m_strProjectName );

	m_componentPath = binDirectory / m_strComponentName;
	std::cout << "Attempting to load component: " << m_componentPath.string() << std::endl;

    m_pPlugin = boost::dll::import< megastructure::EGComponent >(   // type of imported symbol is located between `<` and `>`
            m_componentPath,                       // path to the library and library name
            "g_pluginSymbol",                                       // name of the symbol to import
            boost::dll::load_mode::append_decorations               // makes `libmy_plugin_sum.so` or `my_plugin_sum.dll` from `my_plugin_sum`
        );
    
	
	//attempt to initialise the programs memory
	
	
}

Program::~Program()
{

}

void Program::run()
{
    m_pPlugin->Cycle();
}

}