

#include "master.hpp"

#include "activities.hpp"

#include "schema/projectTree.hpp"

namespace master
{
	
	Master::Master( Environment& environment, const boost::filesystem::path& workspacePath, const std::string& strPort )
		:	m_environment( environment ),
			m_workspacePath( workspacePath ),
			m_server( megastructure::TCPLocalSocketName( strPort ) )
	{
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
		
		m_zeromqServer = std::thread( [ &server, &queue ]()
		{
			megastructure::readServer( server, queue );
		});
		
		m_queue.startActivity( new EnrollActivity( *this ) );
		m_queue.startActivity( new RouteEGProtocolActivity( *this ) );
	}
	
	
	Master::~Master()
	{
		m_queue.stop();
		m_server.stop();
		m_zeromqServer.join();
	}
	
	void Master::calculateNetworkAddressTable( std::shared_ptr< ProjectTree > pProjectTree )
	{
		m_pNetworkAddressTable.reset( 
			new megastructure::NetworkAddressTable( 
					getClients(), 
					pProjectTree ) );
	}
}