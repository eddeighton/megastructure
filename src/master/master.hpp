
#ifndef MASTER_26_APRIL_2020
#define MASTER_26_APRIL_2020

#include "megastructure/coordinator.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"
#include "megastructure/networkAddressTable.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

#include "schema/project.hpp"

#include <thread>

class ProjectTree;

namespace master
{
	
	class Master
	{
	public:
		Master( Environment& environment, const boost::filesystem::path& workspacePath, const std::string& strPort );
		~Master();
		
		void startActivity( megastructure::Activity::Ptr pActivity )
		{
			m_queue.startActivity( pActivity );
		}
		void startActivity( megastructure::Activity* pActivity )
		{
			m_queue.startActivity( megastructure::Activity::Ptr( pActivity ) );
		}
		void activityComplete( megastructure::Activity::Ptr pActivity )
		{
			m_queue.activityComplete( pActivity );
		}
		
		bool send( const megastructure::Message& message, std::uint32_t uiClient )
		{
			return m_server.send( message, uiClient );
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
		const megastructure::ClientMap::ClientIDMap& getClients() const { return m_clients.getClients(); }
		
		void setActiveProgramName( const std::string& strActiveProgramName )
		{
			m_strActiveProgramName = strActiveProgramName;
		}
		const std::string& getActiveProgramName() const
		{
			return m_strActiveProgramName;
		}
		megastructure::NetworkAddressTable::Ptr 
			getNetworkAddressTable() const { return m_pNetworkAddressTable; }
		
		void calculateNetworkAddressTable( std::shared_ptr< ProjectTree > pProjectTree );
		
		const boost::filesystem::path& getWorkspace() const { return m_workspacePath; }
		Environment& getEnvironment() const { return m_environment; }
	private:
		Environment& m_environment;
		boost::filesystem::path m_workspacePath;
		
		megastructure::ClientMap m_clients;
		megastructure::Queue m_queue;
		megastructure::Server m_server;
		
		std::thread m_zeromqServer;
		
		std::string m_strActiveProgramName;
		megastructure::NetworkAddressTable::Ptr m_pNetworkAddressTable;
		
	};
	
	
}


#endif //MASTER_26_APRIL_2020