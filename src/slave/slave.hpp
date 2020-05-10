

#ifndef SLAVE_26_APRIL_2020
#define SLAVE_26_APRIL_2020

#include "slave/hostMap.hpp"

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"
#include "megastructure/buffer.hpp"
#include "megastructure/networkAddressTable.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

#include "schema/project.hpp"

#include "common/assert_verify.hpp"

#include <boost/filesystem/path.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <ostream>
#include <map>

class ProjectTree;

namespace slave
{
	void getWorkspaceAndSlaveNameFromPath( const boost::filesystem::path& thePath, 
		boost::filesystem::path& workspace, std::string& strSlaveName );
	
	class Slave
	{
	public:
        using SharedBufferMap = std::map< std::string, megastructure::SharedBufferImpl::Ptr >;
		
		Slave( Environment& environment,
			const std::string& strMasterIP, 
			const std::string strMasterPort, 
			const std::string strSlaveMegaPort, 
			const std::string strSlaveEGPort, 
			const boost::filesystem::path& workspacePath,
			const std::string& strSlaveName );
		~Slave();
		
		const std::string& getName() const { return m_strSlaveName; }
		const boost::filesystem::path& getWorkspace() const { return m_workspacePath; }
		Environment& getEnvironment() const { return m_environment; }
		const HostMap& getHosts() const { return m_hostMap; }
		
		megastructure::NetworkAddressTable::Ptr 
			getNATRequests() const { return m_pNetworkAddressTableRequests; }
		//megastructure::NetworkAddressTable::Ptr 
		//	getNATResponses() const { return m_pNetworkAddressTableResponses; }
		
		std::future< bool > getEnrollment() { return m_masterEnrollPromise.get_future(); }
		const std::string& getActiveProgramName() const { return m_strActiveProgramName; }
		
		void listClients( std::ostream& os ) const
		{
			m_hostMap.listClients( os );
		}
		
		void setEnrollment( bool bEnrolledWithMaster ) { m_masterEnrollPromise.set_value( bEnrolledWithMaster ); }
		void setActiveProgramName( const std::string& strActiveProgramName ) { m_strActiveProgramName = strActiveProgramName; }
		void setHostMap( const HostMap& newHostMap ) { m_hostMap = newHostMap; }
		
		bool sendMaster( const megastructure::Message& message );
		bool sendHost( const megastructure::Message& message, std::uint32_t uiClient );
		bool sendHostEG( const megastructure::Message& message, std::uint32_t uiClient );
		
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
			m_hostMap.removeClient( uiClient );
		}
		bool enroll( const std::string& strName, std::uint32_t clientID, const std::string& strUnique )
		{
			return m_hostMap.enroll( strName, clientID, strUnique );
		}
		bool enrolleg( std::uint32_t clientID, const std::string& strUnique )
		{
			return m_hostMap.enrolleg( clientID, strUnique );
		}
		
        std::string getSharedBufferName( const std::string& strBufferName, std::size_t szSize );
        
		void calculateNetworkAddressTable( std::shared_ptr< ProjectTree > pProjectTree );
		
	private:
		Environment& m_environment;
	
		std::promise< bool > m_masterEnrollPromise;
		std::string m_strActiveProgramName;
		
		boost::filesystem::path m_workspacePath;
		std::string m_strSlaveName;
		HostMap m_hostMap;
        
        SharedBufferMap m_sharedBuffers;
		
		megastructure::NetworkAddressTable::Ptr m_pNetworkAddressTableRequests;
		//megastructure::NetworkAddressTable::Ptr m_pNetworkAddressTableResponses;
		
		megastructure::Queue m_queue;
		megastructure::Server m_server;
		megastructure::Server m_egServer;
		megastructure::Client m_client;
		std::thread m_zeromqClient;
		std::thread m_zeromqServer;
		std::thread m_zeromqEGServer;
	};


}

#endif //SLAVE_26_APRIL_2020
