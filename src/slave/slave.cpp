
#include "slave.hpp"

#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"

namespace slave
{

	Slave::Slave( const std::string& strMasterIP, const std::string strMasterPort, const std::string& strSlavePort, const std::string& strSlaveName )
		:	m_strSlaveName( strSlaveName ),
			m_server( strSlavePort ),
			m_client( strMasterIP, strMasterPort )
	{
		
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
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
		
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new MasterEnrollActivity( *this ) );
		m_queue.startActivity( new HostEnrollActivity( *this ) );
	}
	
	Slave::~Slave()
	{
		m_queue.stop();
		
		//m_zeromqClient.join();
		//m_zeromqServer.join();
		
	}
	
	
}