
#include "slave.hpp"

#include "megastructure/mega.hpp"
#include "megastructure/log.hpp"

#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"
#include "activitiesEG.hpp"

#include "schema/projectTree.hpp"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include "boost/filesystem.hpp"       

namespace slave
{
    
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	void getWorkspaceAndSlaveNameFromPath( const boost::filesystem::path& thePath, 
		boost::filesystem::path& workspace, std::string& strSlaveName )
	{
		const boost::filesystem::path canonPath = 
			boost::filesystem::edsCannonicalise(
				boost::filesystem::absolute( thePath ) );
		
		VERIFY_RTE_MSG( boost::filesystem::exists( canonPath ), 
			"Specified slave path does not exist: " << thePath.string() );
		
		workspace = canonPath.parent_path().parent_path();
		strSlaveName = canonPath.stem().string();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	Slave::Slave( 	Environment& environment,
					const std::string& strMasterIP, 
					const std::string strMasterPort, 
					const std::string strSlaveMegaPort, 
					const std::string strSlaveEGPort, 
					const boost::filesystem::path& workspacePath,
					const std::string& strSlaveName  )
		:	m_environment( environment ),
			m_server( megastructure::TCPLocalSocketName( strSlaveMegaPort ) ),
			m_egServer( megastructure::TCPLocalSocketName( strSlaveEGPort ) ),
			m_client( megastructure::TCPRemoteSocketName( strMasterIP, strMasterPort ) ),
			m_workspacePath( workspacePath ),
			m_strSlaveName( strSlaveName )
	{
        SPDLOG_INFO( "Created megastructure slave with master ip:{} master port:{} slave mega port:{} slave eg port:{} workspace path:{} slave name:{}",
            strMasterIP, strMasterPort, strSlaveMegaPort, strSlaveEGPort, workspacePath.string(), strSlaveName );
		
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
		megastructure::Server& egServer = m_egServer;
		megastructure::Client& client = m_client;
	
		m_zeromqClient = std::thread(
			[ &client, &queue ]()
			{
				megastructure::readClient( client, queue );
			});
			
		m_zeromqServer = std::thread(
			[ &server, &queue ]()
			{
				megastructure::readServer( server, queue );
			});
			
		m_zeromqEGServer = std::thread(
			[ &egServer, &queue ]()
			{
				megastructure::readServer( egServer, queue );
			});
		
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new MasterEnrollActivity( *this ) );
		m_queue.startActivity( new HostEnrollActivity( *this ) );
		m_queue.startActivity( new LoadProgramActivity( *this ) );
		m_queue.startActivity( new HostBufferActivity( *this ) );
		m_queue.startActivity( new RouteEGProtocolActivity( *this ) );
		m_queue.startActivity( new ConfigActivity( *this ) );
	}
	
	Slave::~Slave()
	{
		m_queue.stop();
		m_server.stop();
		m_egServer.stop();
		m_client.stop();
		m_zeromqClient.join();
		m_zeromqServer.join();
		m_zeromqEGServer.join();
	}
	
	bool Slave::sendMaster( const megastructure::Message& message )
	{
		return m_client.send( message );
	}
	
	bool Slave::sendHost( const megastructure::Message& message, std::uint32_t uiClient )
	{
		bool bSuccess = m_server.send( message, uiClient );
		if( !bSuccess )
		{
			removeClient( uiClient );
		}
		return bSuccess;
	}
	
	bool Slave::sendHostEG( const megastructure::Message& message, std::uint32_t uiClient )
	{
		bool bSuccess = m_egServer.send( message, uiClient );
		if( !bSuccess )
		{
			removeClient( uiClient );
		}
		return bSuccess;
	}
	
    std::string Slave::getSharedBufferName( const std::string& strBufferName, std::size_t szSize )
    {
        SharedBufferMap::iterator iFind = m_sharedBuffers.find( strBufferName );
        if( iFind != m_sharedBuffers.end() )
        {
            megastructure::SharedBufferImpl::Ptr pBuffer = iFind->second;
            
            //attempt to reuse existing buffer
            if( pBuffer->getSize() == szSize )
            {
                return pBuffer->getSharedName();
            }
        }
        
        const std::string strSharedBufferName = 
			megastructure::generateUniqueString();
        
        m_sharedBuffers[ strBufferName ] = 
            std::make_shared< megastructure::SharedBufferImpl >( strBufferName, strSharedBufferName, szSize );
        SPDLOG_TRACE( "Created shared buffer: {} shared name: {} size: {}", strBufferName, strSharedBufferName, szSize );
        
        return strSharedBufferName;
    }
	
	void Slave::calculateNetworkAddressTable( std::shared_ptr< ProjectTree > pProjectTree )
	{
        if( boost::filesystem::exists( pProjectTree->getAnalysisFileName() ) )
        {
			//requests are always routed to the mega structure socket
			{
				megastructure::NetworkAddressTable::ClientMap clientMap;
			
				for( const auto& i : m_hostMap.getHostNameMapping() )
				{
					clientMap.insert( std::make_pair( i.first, i.second.getMegaClientID() ) );
				}
			
				m_pNetworkAddressTableRequests.reset( 
					new megastructure::NetworkAddressTable( 
							clientMap, 
							getName(), 
							pProjectTree ) );
			}
			
			//responses, events and errors are always routed to the eg socket
			//{
			//	megastructure::NetworkAddressTable::ClientMap clientMap;
			//
			//	for( const auto& i : m_hostMap.getHostNameMapping() )
			//	{
			//		clientMap.insert( std::make_pair( i.first, i.second.getEGClientID() ) );
			//	}
			//
			//	m_pNetworkAddressTableResponses.reset( 
			//		new megastructure::NetworkAddressTable( 
			//				clientMap, 
			//				getName(), 
			//				pProjectTree ) );
			//}
        }
        else
        {
            m_pNetworkAddressTableRequests.reset();
            //m_pNetworkAddressTableResponses.reset();
        }
	}
}