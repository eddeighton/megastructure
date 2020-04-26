

#ifndef SLAVE_26_APRIL_2020
#define SLAVE_26_APRIL_2020

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <ostream>

namespace slave
{
	class Slave
	{
	public:
		
		Slave( const std::string& strMasterIP, 
			const std::string strMasterPort, 
			const std::string& strSlavePort, 
			const std::string& strSlaveName );
		~Slave();
		
		bool sendMaster( megastructure::Message& message )
		{
			return m_client.send( message );
		}
		
		bool sendHost( megastructure::Message& message, std::uint32_t uiClient )
		{
			return m_server.send( message, uiClient );
		}
		
		void startActivity( megastructure::Activity::Ptr pActivity )
		{
			m_queue.startActivity( pActivity );
		}
		void startActivity( megastructure::Activity* pActivity )
		{
			m_queue.startActivity( pActivity );
		}
		void activityComplete( megastructure::Activity::Ptr pActivity )
		{
			m_queue.activityComplete( pActivity );
		}
		
		void removeClient( std::uint32_t uiClient )
		{
			m_clients.removeClient( uiClient );
		}
		bool getClientID( const std::string& strName, std::uint32_t& clientID ) const
		{
			return m_clients.getClientID( strName, clientID );
		}
		bool enroll( const std::string& strName, std::uint32_t clientID )
		{
			return m_clients.enroll( strName, clientID );
		}
		void listClients( std::ostream& os )
		{
			const megastructure::ClientMap::ClientIDMap& clients = m_clients.getClients();
			for( megastructure::ClientMap::ClientIDMap::const_iterator 
				i = clients.begin(), iEnd = clients.end();
				i!=iEnd; ++i )
			{
				os << "Client: " << i->first << " id: " << i->second << std::endl;
			}
		}
		
		const std::string& getName() const { return m_strSlaveName; }
		
		const megastructure::ClientMap::ClientIDMap& getClients() const { return m_clients.getClients(); }
		
	private:
		std::string m_strSlaveName;
		megastructure::ClientMap m_clients;
		megastructure::Queue m_queue;
		megastructure::Server m_server;
		megastructure::Client m_client;
		std::thread m_zeromqClient;
		std::thread m_zeromqServer;
	};


}

#endif //SLAVE_26_APRIL_2020
