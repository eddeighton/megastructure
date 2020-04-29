

#ifndef SLAVE_26_APRIL_2020
#define SLAVE_26_APRIL_2020

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"

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

namespace slave
{
	
	template< typename Container, typename ValueType = typename Container::value_type::second_type >
	void eraseByMapSecond( Container& container, const ValueType& value )
	{
		for( auto i = container.begin(), iEnd = container.end(); i!=iEnd;  )
		{
			if( i->second == value )
				i = container.erase( i );
			else
				++i;
		}
	}
	
	class HostMap
	{
	public:
		using HostProcessNameMap = std::multimap< std::string, std::uint32_t >;
	
		void removeClient( std::uint32_t uiClient )
		{
			eraseByMapSecond( m_clientProcessNameMap, uiClient );
			eraseByMapSecond( m_hostnameMapping, uiClient );
		}
		bool enroll( const std::string& strProcessName, std::uint32_t clientID )
		{
			m_clientProcessNameMap.insert( std::make_pair( strProcessName, clientID ) );
			return true;
		}
		void listClients( std::ostream& os )
		{
			for( auto i : m_clientProcessNameMap )
			{
				std::cout << "Client Process Name: " << i.first << " id: " << i.second << std::endl;
			}
		}
	
		const HostProcessNameMap& getEnrolledHosts() const { return m_clientProcessNameMap; }
	
	private:
		std::multimap< std::string, std::uint32_t > m_clientProcessNameMap;
		std::map< std::string, std::uint32_t > m_hostnameMapping;
	};
	
	class Slave
	{
	public:
		
		Slave( Environment& environment,
			const std::string& strMasterIP, 
			const std::string strMasterPort, 
			const std::string& strSlavePort, 
			const boost::filesystem::path& strSlavePath );
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
			m_hostMap.removeClient( uiClient );
		}
		bool enroll( const std::string& strName, std::uint32_t clientID )
		{
			return m_hostMap.enroll( strName, clientID );
		}
		void listClients( std::ostream& os )
		{
			m_hostMap.listClients( os );
		}
		
		const std::string& getName() const { return m_strSlaveName; }
		const boost::filesystem::path& getWorkspace() const { return m_workspacePath; }
		
		std::future< bool > getEnrollment()
		{
			return m_masterEnrollPromise.get_future();
		}
		void setEnrollment( bool bEnrolledWithMaster )
		{
			m_masterEnrollPromise.set_value( bEnrolledWithMaster );
		}
		
		void setActiveProgramName( const std::string& strActiveProgramName )
		{
			m_strActiveProgramName = strActiveProgramName;
		}
		const std::string& getActiveProgramName() const
		{
			return m_strActiveProgramName;
		}
		
		
		Environment& getEnvironment() const { return m_environment; }
		const HostMap& getHosts() const { return m_hostMap; }
		
	private:
		Environment& m_environment;
	
		std::promise< bool > m_masterEnrollPromise;
		std::string m_strActiveProgramName;
		
		boost::filesystem::path m_workspacePath;
		std::string m_strSlaveName;
		HostMap m_hostMap;
		
		megastructure::Queue m_queue;
		megastructure::Server m_server;
		megastructure::Client m_client;
		std::thread m_zeromqClient;
		std::thread m_zeromqServer;
	};


}

#endif //SLAVE_26_APRIL_2020
