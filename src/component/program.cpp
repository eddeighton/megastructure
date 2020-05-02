
#include "program.hpp"

#include "egcomponent/egcomponent.hpp"

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
    
    std::cout << "Attempting initialisation" << std::endl;
	//initialise the programs memory
	m_pPlugin->Initialise( this, this );
    
    std::cout << "Initialisation complete" << std::endl;
}

Program::~Program()
{
    m_pPlugin->Uninitialise();
}

//MemorySystem

SharedBuffer* Program::getSharedBuffer( const char* pszName, std::size_t szSize )
{
    std::string strBufferName = pszName;
    
    //std::cout << "Requesting shared buffer: " << pszName << " size: " << szSize << std::endl;
    std::future< std::string > strFuture = m_component.getSharedBuffer( strBufferName, szSize );
    const std::string strBufferSharedName = strFuture.get();
    //std::cout << "Got shared buffer name of: " << strBufferSharedName << std::endl;
    
    SharedBufferMap::iterator iFind = m_sharedBuffers.find( strBufferName );
    if( iFind != m_sharedBuffers.end() )
    {
        SharedBufferImpl::Ptr pSharedBuffer = iFind->second;
        if( pSharedBuffer->getSize() == szSize && 
            pSharedBuffer->getSharedName() == strBufferSharedName )
        {
            return pSharedBuffer.get();
        }
    }
    
    //acquire new buffer
    SharedBufferImpl::Ptr pSharedBuffer =
        std::make_shared< SharedBufferImpl >( strBufferName, strBufferSharedName, szSize );
    m_sharedBuffers[ strBufferName ] = pSharedBuffer;
    //std::cout << "Loaded shared buffer: " << strBufferName << 
    //    " size: " << szSize << " " << strBufferSharedName << std::endl;
    
    return pSharedBuffer.get();
}

LocalBuffer* Program::getLocalBuffer( const char* pszName, std::size_t szSize )
{
    std::string strBufferName = pszName;
    CacheBufferMap::iterator iFind = m_cacheBuffers.find( strBufferName );
    if( iFind == m_cacheBuffers.end() )
    {
        LocalBufferImpl::Ptr pBuffer = std::make_shared< LocalBufferImpl >( strBufferName, szSize );
        m_cacheBuffers.insert( std::make_pair( std::string( pszName ), pBuffer ) );
        //std::cout << "Local buffer: " << pszName << " size: " << szSize << std::endl;
        return pBuffer.get();
    }
    else
    {
        THROW_RTE( "Duplicate buffer requested: " << strBufferName );
    }
}

//MegaProtocol
/*
boost::fibers::future< std::string > Program::Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance )
{
    boost::fibers::future< std::string > result;
    return result;
}*/
void Program::Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer )
{
}
void Program::Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer )
{
}
void Program::Pause( std::uint32_t uiActionType, std::uint32_t uiInstance )
{
}
void Program::Resume( std::uint32_t uiActionType, std::uint32_t uiInstance )
{
}
void Program::Stop( std::uint32_t uiActionType, std::uint32_t uiInstance )
{
}
        
        
        
void Program::run()
{
    m_pPlugin->Cycle();
}

}