

#ifndef SLAVE_26_APRIL_2020
#define SLAVE_26_APRIL_2020

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"
#include "megastructure/buffer.hpp"

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
		using HostNameMap = std::map< std::string, std::uint32_t >;
		using ProcessNameToHostNameMap = std::multimap< std::string, std::string >;
	
		HostMap();
		
		HostMap( const HostMap& oldHostMapping, 
			ProcessNameToHostNameMap processNameToHostNameMap, 
			std::vector< std::uint32_t >& unmappedClients, 
			std::vector< std::string >& unmappedHostNames );
			
		const HostProcessNameMap& getEnrolledHosts() const { return m_clientProcessNameMap; }
		const HostNameMap& getHostNameMapping() const { return m_hostnameMapping; }
				void removeClient( std::uint32_t uiClient );
		void listClients( std::ostream& os ) const;
		bool enroll( const std::string& strProcessName, std::uint32_t clientID );
	
	private:
		std::multimap< std::string, std::uint32_t > m_clientProcessNameMap;
		std::map< std::string, std::uint32_t > m_hostnameMapping;
		
	};
	
	class Slave
	{
	public:
        using SharedBufferMap = std::map< std::string, megastructure::SharedBufferImpl::Ptr >;
		
		Slave( Environment& environment,
			const std::string& strMasterIP, 
			const std::string strMasterPort, 
			const std::string& strSlavePort, 
			const boost::filesystem::path& strSlavePath );
		~Slave();
		
		const std::string& getName() const { return m_strSlaveName; }
		const boost::filesystem::path& getWorkspace() const { return m_workspacePath; }
		Environment& getEnvironment() const { return m_environment; }
		const HostMap& getHosts() const { return m_hostMap; }
		
		std::future< bool > getEnrollment() { return m_masterEnrollPromise.get_future(); }
		const std::string& getActiveProgramName() const { return m_strActiveProgramName; }
		
		void listClients( std::ostream& os ) const
		{
			m_hostMap.listClients( os );
		}
		
		void setEnrollment( bool bEnrolledWithMaster ) { m_masterEnrollPromise.set_value( bEnrolledWithMaster ); }
		void setActiveProgramName( const std::string& strActiveProgramName ) { m_strActiveProgramName = strActiveProgramName; }
		void setHostMap( const HostMap& newHostMap ) { m_hostMap = newHostMap; }
		
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
		
        std::string getSharedBufferName( const std::string& strBufferName, std::size_t szSize );
        
	private:
		Environment& m_environment;
	
		std::promise< bool > m_masterEnrollPromise;
		std::string m_strActiveProgramName;
		
		boost::filesystem::path m_workspacePath;
		std::string m_strSlaveName;
		HostMap m_hostMap;
        
        SharedBufferMap m_sharedBuffers;
		
		megastructure::Queue m_queue;
		megastructure::Server m_server;
		megastructure::Client m_client;
		std::thread m_zeromqClient;
		std::thread m_zeromqServer;
	};


}

#endif //SLAVE_26_APRIL_2020
