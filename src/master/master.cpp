

#include "master.hpp"

#include "activities.hpp"

namespace master
{
	
	Master::Master( const std::string& strPort )
		:	m_server( strPort )
	{
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
		
		m_zeromqServer = std::thread( [ &server, &queue ]()
		{
			megastructure::readServer( server, queue );
		});
		
		m_queue.startActivity( new EnrollActivity( *this ) );
	}
	
	
	Master::~Master()
	{
		m_queue.stop();
		m_server.stop();
		m_zeromqServer.join();
	}
	
}