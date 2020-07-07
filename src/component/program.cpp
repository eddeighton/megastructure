
#include "program.hpp"

#include "egcomponent/egcomponent.hpp"

#include "schema/projectTree.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include "boost/dll/import.hpp" // for import_alias


namespace megastructure
{
	
std::string getComponentName( const std::string& strCoordinator, const std::string& strHostName, const std::string& strProjectName )
{
	std::ostringstream os;
	//for now always load the debug dll - d post fix
	os << strCoordinator << '_' << strHostName << '_' << strProjectName << "d.dll";
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
        m_strProjectName( strProjectName ),
		m_pEncodeDecode( nullptr )
{
	
	try
	{
		m_pProjectTree = 
			std::make_shared< ProjectTree >( 
				m_component.getEnvironment(), 
				m_component.getSlaveWorkspacePath(), 
				strProjectName );
	}
	catch( std::exception& ex )
	{
		std::cout << "Error attempting to load project tree for: " << 
			m_component.getSlaveWorkspacePath().string() << " project: " << 
			strProjectName << " : " << ex.what() << std::endl;
		throw;
	}
	
    if( boost::filesystem::exists( m_pProjectTree->getAnalysisFileName() ) )
    {
	    m_pNetworkAddressTable.reset( 
		    new NetworkAddressTable( 
			    m_component.getSlaveName(), 
			    strHostName, 
			    m_pProjectTree ) );
				
		m_pProgramTable.reset(
			new ProgramTypeTable( 
				m_pProjectTree ) );
    }
    else
    {
        m_pNetworkAddressTable.reset();
		m_pProgramTable.reset();
    }
	
    const boost::filesystem::path binDirectory = getBinFolderForProject( m_component.getSlaveWorkspacePath(), m_strProjectName );
    m_strComponentName = getComponentName( m_component.getSlaveName(), m_strHostName, m_strProjectName );

	m_componentPath = binDirectory / m_strComponentName;
	std::cout << "Attempting to load component: " << m_componentPath.string() << std::endl;

    m_pPlugin = boost::dll::import< megastructure::EGComponent >(   // type of imported symbol is located between `<` and `>`
            m_componentPath,                       					// path to the library and library name
            "g_pluginSymbol",                                       // name of the symbol to import
            boost::dll::load_mode::append_decorations               // makes `libmy_plugin_sum.so` or `my_plugin_sum.dll` from `my_plugin_sum`
        );
    
    std::cout << "Attempting initialisation" << std::endl;
	//initialise the programs memory
	m_pPlugin->Initialise( m_pEncodeDecode, this, this );
	
	VERIFY_RTE_MSG( m_pEncodeDecode, "Did not get encode decode interface from program: " << m_componentPath.string() );
    
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

void Program::readBuffer( std::int32_t iType, std::uint32_t uiInstance, std::string& strBuffer )
{
	msgpack::sbuffer buffer;
	msgpack::packer< msgpack::sbuffer > packer( &buffer );
	m_pEncodeDecode->encode( iType, uiInstance, packer );
	strBuffer.assign( buffer.data(), buffer.size() );
}

void Program::writeBuffer( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer )
{
	msgpack::unpacker decoder;
	decoder.reserve_buffer( strBuffer.size() );
	memcpy( decoder.buffer(), strBuffer.data(), strBuffer.size() );
	decoder.buffer_consumed( strBuffer.size() );
	m_pEncodeDecode->decode( iType, uiInstance, decoder );
}
		
//MegaProtocol
/*
bool Program::receive( std::int32_t& iType, std::size_t& uiInstance, std::uint32_t& uiTimestamp )
{
	megastructure::Message message;
	
	while( m_component.readeg( message ) )
	{
		if( message.has_eg_msg() )
		{
			const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
			
			if( egMsg.has_request() )
			{
				THROW_RTE( "Program received eg request on eg socket" );
			}
			else if( egMsg.has_response() )
			{
				const megastructure::Message::EG_Msg::Response& egResponse = egMsg.response();
				
				iType 		= egMsg.type();
				uiInstance 	= egMsg.instance();
				uiTimestamp = egMsg.cycle();
				
				//first update the cache with the actual value
				writeBuffer( iType, uiInstance, egResponse.value() );
				
				std::cout << "Program received response type: " << iType << 
					" instance: " << uiInstance << " timestamp: " << uiTimestamp << std::endl;
				
				return true;
			}
			else if( egMsg.has_error() )
			{
				const megastructure::Message::EG_Msg::Error& egError = egMsg.error();
				
				iType 		= egMsg.type();
				uiInstance 	= egMsg.instance();
				uiTimestamp = egMsg.cycle();
				
				//first update the cache with the actual value
				//writeBuffer( iType, uiInstance, egResponse.value() );
				
				std::cout << "Program received error type: " << iType << 
					" instance: " << uiInstance << " timestamp: " << uiTimestamp << std::endl;
				
				return true;
			}
			else if( egMsg.has_event() )
			{
				const megastructure::Message::EG_Msg::Event& egEvent = egMsg.event();
				
				iType 		= egMsg.type();
				uiInstance 	= egMsg.instance();
				uiTimestamp = egMsg.cycle();
				
				std::cout << "Program received event type: " << iType << 
					" instance: " << uiInstance << " timestamp: " << uiTimestamp << std::endl;
				
				return true;
			}
			else
			{
				THROW_RTE( "Unknown eg message type" );
			}
		}
		else
		{
			THROW_RTE( "Did not receive eg msg on eg socket" );
		}
	}
	
	return false;
}

void Program::send( const char* type, std::size_t timestamp, const void* value, std::size_t size )
{
}
*/
		
//eg
/*
std::string Program::egRead( std::int32_t iType, std::uint32_t uiInstance )
{
	if( m_pNetworkAddressTable->getClientForType( iType ) == NetworkAddressTable::SelfID )
	{
		//write the buffer directly
		std::string strBuffer;
		readBuffer( iType, uiInstance, strBuffer );
		return strBuffer;
	}
	else
	{
		Message message;
		{
			Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
			pEGMsg->set_type( iType );
			pEGMsg->set_instance( uiInstance );
			pEGMsg->set_cycle( 0 );
			
			Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
			Message::EG_Msg::Request::Read* pRead = pRequest->mutable_read();
		}
		m_component.sendeg( message );
		std::cout << "Sent eg read request type: " << iType << " instance: " << uiInstance << std::endl;
		
		//m_pPlugin->WaitForReadResponse( iType, uiInstance );
		
		std::string strBuffer;
		readBuffer( iType, uiInstance, strBuffer );
		return strBuffer;
	}
}

void Program::egWrite( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer )
{
	if( m_pNetworkAddressTable->getClientForType( iType ) == NetworkAddressTable::SelfID )
	{
		//write the buffer directly
		writeBuffer( iType, uiInstance, strBuffer );
	}
	else
	{
		//send the write request
		Message message;
		{
			Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
			pEGMsg->set_type( iType );
			pEGMsg->set_instance( uiInstance );
			pEGMsg->set_cycle( 0 );
			
			Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
			Message::EG_Msg::Request::Write* pWrite = pRequest->mutable_write();
			pWrite->set_value( strBuffer );
		}
		m_component.sendeg( message );
	}
}

void Program::egCall( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer )
{
	if( m_pNetworkAddressTable->getClientForType( iType ) == NetworkAddressTable::SelfID )
	{
		
	}
	else
	{
		//send the write request
		
		
	}
}*/
  
void Program::run()
{
    m_pPlugin->Cycle();
}

}