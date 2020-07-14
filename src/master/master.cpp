

#include "master.hpp"

#include "activities.hpp"

#include "megastructure/log.hpp"

#include "schema/projectTree.hpp"

#include "spdlog/spdlog.h"

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
        
        SPDLOG_INFO( "Master created workspace:{} port:{}", workspacePath.string(), strPort );
	}
	
	
	Master::~Master()
	{
		m_queue.stop();
		m_server.stop();
		m_zeromqServer.join();
        
        SPDLOG_INFO( "Master shutdown" );
	}
	
	void Master::calculateNetworkAddressTable( std::shared_ptr< ProjectTree > pProjectTree )
	{
        if( boost::filesystem::exists( pProjectTree->getAnalysisFileName() ) )
        {
            SPDLOG_INFO( "Master calculating new address table" );
		    m_pNetworkAddressTable.reset( 
			    new megastructure::NetworkAddressTable( 
					    getClients(), pProjectTree ) );
        }
        else
        {
            SPDLOG_INFO( "Master reset address table" );
            m_pNetworkAddressTable.reset();
        }
	}
}