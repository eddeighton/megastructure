
#include "megastructure/program.hpp"
#include "megastructure/log.hpp"

#include "egcomponent/egcomponent.hpp"

#include "schema/projectTree.hpp"

#include "eg_compiler/sessions/implementation_session.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include "boost/dll/import.hpp" // for import_alias


namespace megastructure
{
	
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
                m_component.getSlaveName(),
                m_strHostName,
				strProjectName );
	}
	catch( std::exception& ex )
	{
        SPDLOG_ERROR( "Error attempting to load project tree for: {} project: {} error: {}",
            m_component.getSlaveWorkspacePath().string(),
            strProjectName, ex.what() );
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
	
    const boost::filesystem::path binDirectory = 
        getBinFolderForProject( m_component.getSlaveWorkspacePath(), m_strProjectName );
    m_strComponentName = m_pProjectTree->getComponentFileNameExt( false );

	m_componentPath = binDirectory / m_strComponentName;
    SPDLOG_INFO( "Loading component: {}", m_componentPath.string() );

    m_pPlugin = boost::dll::import< megastructure::EGComponent >(   // type of imported symbol is located between `<` and `>`
            m_componentPath,                       					// path to the library and library name
            "g_pluginSymbol",                                       // name of the symbol to import
            boost::dll::load_mode::append_decorations               // makes `libmy_plugin_sum.so` or `my_plugin_sum.dll` from `my_plugin_sum`
        );
        
    //initialise the program
    m_pPlugin->Initialise( m_component.getHostInterface(), 
        m_pEncodeDecode, this, this, this,
        m_pProjectTree->getAnalysisFileName().string().c_str() );
	
	VERIFY_RTE_MSG( m_pEncodeDecode, "Did not get encode decode interface from program: " << m_componentPath.string() );
    
    SPDLOG_INFO( "Initialisation complete" );
}

Program::~Program()
{
    m_pPlugin->Uninitialise();
}

//MemorySystem

SharedBuffer* Program::getSharedBuffer( const char* pszName, std::size_t szSize )
{
    std::string strBufferName = pszName;
    
    SPDLOG_TRACE( "Requesting shared buffer: {} size: {}", pszName, szSize );
    std::future< std::string > strFuture = m_component.getSharedBuffer( strBufferName, szSize );
    const std::string strBufferSharedName = strFuture.get();
    
    SharedBufferMap::iterator iFind = m_sharedBuffers.find( strBufferName );
    if( iFind != m_sharedBuffers.end() )
    {
        SharedBufferImpl::Ptr pSharedBuffer = iFind->second;
        if( pSharedBuffer->getSize() == szSize && 
            pSharedBuffer->getSharedName() == strBufferSharedName )
        {
            SPDLOG_TRACE( "Found existing matching shared buffer: {}", strBufferName );
            return pSharedBuffer.get();
        }
    }
    
    //acquire new buffer
    SharedBufferImpl::Ptr pSharedBuffer =
        std::make_shared< SharedBufferImpl >( strBufferName, strBufferSharedName, szSize );
    m_sharedBuffers[ strBufferName ] = pSharedBuffer;
    SPDLOG_TRACE( "Created shared buffer name: {} shared name: {} size: {} ", strBufferName, strBufferSharedName, szSize );
    
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
        SPDLOG_TRACE( "Local buffer: {} size: {}", pszName, szSize );
        return pBuffer.get();
    }
    else
    {
        SPDLOG_ERROR( "Duplicate buffer requested: {}", strBufferName );
        THROW_RTE( "Duplicate buffer requested: " << strBufferName );
    }
}

void Program::readRequest( std::int32_t iType, std::uint32_t uiInstance, std::string& strBuffer )
{
	msgpack::sbuffer buffer;
	msgpack::packer< msgpack::sbuffer > packer( &buffer );
	m_pEncodeDecode->encode( iType, uiInstance, packer );
	strBuffer.assign( buffer.data(), buffer.size() );
}
void Program::readResponse( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer )
{
	msgpack::unpacker decoder;
	decoder.reserve_buffer( strBuffer.size() );
	memcpy( decoder.buffer(), strBuffer.data(), strBuffer.size() );
	decoder.buffer_consumed( strBuffer.size() );
	m_pEncodeDecode->decode( iType, uiInstance, decoder );
}
void Program::writeRequest( const std::string& strBuffer )
{
    msgpack::unpacker decoder;
    decoder.reserve_buffer( strBuffer.size() );
    memcpy( decoder.buffer(), strBuffer.data(), strBuffer.size() );
    decoder.buffer_consumed( strBuffer.size() );
    
	m_pEncodeDecode->decode( decoder );
}
		
void Program::releaseLocks()
{
    for( const ComponentLockInfo& lockInfo : m_componentLocks )
    {
        Message message;
        {
            Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
            pEGMsg->set_type( lockInfo.first );
            pEGMsg->set_instance( 0 );
            pEGMsg->set_cycle( lockInfo.second );
            Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
            Message::EG_Msg::Request::Unlock* pUnlock = pRequest->mutable_unlock();
        }
        SPDLOG_TRACE( "Requesting un lock on: {} timestamp: {}", lockInfo.first, lockInfo.second );
        m_component.sendeg( message );
    }
    m_componentLocks.clear();
}

//MegaProtocol
eg::TimeStamp Program::readlock( eg::TypeID iComponentType, std::uint32_t uiTimestamp )
{
    {
        Message message;
        {
            Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
            pEGMsg->set_type( iComponentType );
            pEGMsg->set_instance( 0 );
            pEGMsg->set_cycle( uiTimestamp );
            
            Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
            Message::EG_Msg::Request::Lock* pLock = pRequest->mutable_lock();
            pLock->set_read( true );
        }
        SPDLOG_TRACE( "Requesting read lock on: {} timestamp: {}", iComponentType, uiTimestamp );
        m_component.sendeg( message );
    }
    
    {
        megastructure::Message message;
        while( m_component.readegSync( message ) )
        {
            if( message.has_eg_msg() )
            {
                const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
                
                if( egMsg.has_response() )
                {
                    const megastructure::Message::EG_Msg::Response& egResponse = egMsg.response();
                    SPDLOG_TRACE( "Program received read lock response type: {} instance: {} timestamp: {}", 
                        egMsg.type(), egMsg.instance(), egMsg.cycle() );
                    m_componentLocks.insert( ComponentLockInfo{ iComponentType, uiTimestamp } );
                    return egMsg.cycle();
                }
                else if( egMsg.has_error() )
                {
                    SPDLOG_ERROR( "Error requesting read lock on component: {}", iComponentType );
                }
                else
                {
                    SPDLOG_ERROR( "Unexpected response to read lock request" );
                    THROW_RTE( "Unexpected response to read lock request" );
                }
            }
            else
            {
                SPDLOG_ERROR( "Did not receive eg msg on eg socket" );
                THROW_RTE( "Did not receive eg msg on eg socket" );
            }
        }
    }
    return 0;
}

void Program::read( eg::TypeID type, std::uint32_t& uiInstance, std::uint32_t uiTimestamp )
{
    {
        Message message;
        {
            Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
            pEGMsg->set_type( type );
            pEGMsg->set_instance( uiInstance );
            pEGMsg->set_cycle( uiTimestamp );
            
            Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
            Message::EG_Msg::Request::Read* pRead = pRequest->mutable_read();
        }
        SPDLOG_TRACE( "Requesting read on: {} timestamp: {}", type, uiTimestamp );
        m_component.sendeg( message );
    }
    
    {
        megastructure::Message message;
        while( m_component.readegSync( message ) )
        {
            if( message.has_eg_msg() )
            {
                const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
                if( egMsg.has_response() )
                {
                    const megastructure::Message::EG_Msg::Response& egResponse = egMsg.response();
                    SPDLOG_TRACE( "Program received read response type: {} instance: {} timestamp: {}", 
                        egMsg.type(), egMsg.instance(), egMsg.cycle() );
                    readResponse( type, uiInstance, egResponse.value() );
                    break;
                }
                else if( egMsg.has_error() )
                {
                    SPDLOG_ERROR( "Error requesting read on type: {} instance: {}", type, uiInstance );
                }
                else
                {
                    SPDLOG_ERROR( "Unexpected message" );
                    THROW_RTE( "Unexpected message" );
                }
            }
            else
            {
                SPDLOG_ERROR( "Did not receive eg msg on eg socket" );
                THROW_RTE( "Did not receive eg msg on eg socket" );
            }
        }
    }  
}

eg::TimeStamp Program::writelock( eg::TypeID iComponentType, std::uint32_t uiTimestamp )
{
    {
        Message message;
        {
            Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
            pEGMsg->set_type( iComponentType );
            pEGMsg->set_instance( 0 );
            pEGMsg->set_cycle( uiTimestamp );
            
            Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
            Message::EG_Msg::Request::Lock* pLock = pRequest->mutable_lock();
            pLock->set_read( false );
        }
        SPDLOG_TRACE( "Requesting write lock on: {} timestamp: {}", iComponentType, uiTimestamp );
        m_component.sendeg( message );
    }
    
    {
        megastructure::Message message;
        while( m_component.readegSync( message ) )
        {
            if( message.has_eg_msg() )
            {
                const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
                
                if( egMsg.has_response() )
                {
                    const megastructure::Message::EG_Msg::Response& egResponse = egMsg.response();
                    SPDLOG_TRACE( "Program received write lock response type: {} instance: {} timestamp: {}", 
                        egMsg.type(), egMsg.instance(), egMsg.cycle() );
                    m_componentLocks.insert( ComponentLockInfo{ iComponentType, uiTimestamp } );
                    return egMsg.cycle();
                }
                else if( egMsg.has_error() )
                {
                    SPDLOG_ERROR( "Error requesting lock on component: {}", iComponentType );
                }
                else
                {
                    SPDLOG_ERROR( "Unexpected response to write lock request" );
                    THROW_RTE( "Unexpected response to write lock request" );
                }
            }
            else
            {
                SPDLOG_ERROR( "Did not receive eg msg on eg socket" );
                THROW_RTE( "Did not receive eg msg on eg socket" );
            }
        }
    }
    return 0;
}
        
void Program::write( eg::TypeID component, const char* pBuffer, std::size_t szSize, std::uint32_t uiTimestamp )
{
    Message message;
    {
        Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
        pEGMsg->set_type( component );
        pEGMsg->set_instance( 0 );
        pEGMsg->set_cycle( uiTimestamp );
        
        Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
        Message::EG_Msg::Request::Write* pWrite = pRequest->mutable_write();
        pWrite->set_value( pBuffer, szSize );
    }
    SPDLOG_TRACE( "Requesting write on: {} with buffer size of: {}", component, szSize );
    m_component.sendeg( message );
}
        
void* Program::getRoot() const
{
    return m_pPlugin->GetRoot();
}
        
eg::TimeStamp Program::getCurrentCycle() const
{
    return m_pPlugin->GetCurrentCycle();
}

void Program::run()
{
    m_pPlugin->Cycle();
    releaseLocks();
}

void Program::put( const char* type, eg::TimeStamp timestamp, const void* value, std::size_t size )
{
    eg::ReadSession* pDatabase = nullptr;
    
    if( 0 == strcmp( type, "start" ) )
    {
        VERIFY_RTE( sizeof( eg::reference ) == size );
        const eg::reference* pRef = reinterpret_cast< const eg::reference* >( value );
        
        if( pDatabase )
        {
            const eg::concrete::Action* pAction = pDatabase->getConcreteAction( pRef->type );
            SPDLOG_TRACE( "start instance {} timestamp {} type {}", pRef->instance, pRef->timestamp, pAction->getName() );
        }
        else
        {
            SPDLOG_TRACE( "start instance {} timestamp {} type {}", pRef->instance, pRef->timestamp, pRef->type );
        }
    }
    else if( 0 == strcmp( type, "stop" ) )
    {
        VERIFY_RTE( sizeof( eg::reference ) == size );
        const eg::reference* pRef = reinterpret_cast< const eg::reference* >( value );
        if( pDatabase )
        {
            const eg::concrete::Action* pAction = pDatabase->getConcreteAction( pRef->type );
            SPDLOG_TRACE( "stop instance {} timestamp {} type {}", pRef->instance, pRef->timestamp, pAction->getName() );
        }
        else
        {
            SPDLOG_TRACE( "stop instance {} timestamp {} type {}", pRef->instance, pRef->timestamp, pRef->type );
        }
    }
    else if( 0 == strcmp( type, "log" ) )
    {
        SPDLOG_INFO( reinterpret_cast< const char* >( value ) );
    }
    else if( 0 == strcmp( type, "error" ) )
    {
        SPDLOG_ERROR( reinterpret_cast< const char* >( value ) );
    }
    else if( 0 == strcmp( type, "pass" ) )
    {
        SPDLOG_INFO( "pass : {}", reinterpret_cast< const char* >( value ) );
    }
    else if( 0 == strcmp( type, "fail" ) )
    {
        SPDLOG_ERROR( "fail : {}", reinterpret_cast< const char* >( value ) );
    }
    else
    {
        SPDLOG_ERROR( "unknown event : {}", type );
    }
    
}

}