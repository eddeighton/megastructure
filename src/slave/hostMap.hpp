
#ifndef HOSTMAP_06_MAY_2020
#define HOSTMAP_06_MAY_2020

#include <map>
#include <string>
#include <vector>
#include <cstdint>

namespace slave
{

	class Host
	{
	public:
		using ClientID = std::uint32_t;
		
		Host( const std::string& strProcessName, ClientID clientID, const std::string& strUniqueID )
			:	m_strProcessName( strProcessName ),
				m_megaClientID( clientID ),
				m_strUniqueID( strUniqueID )
		{
		}
		
		const std::string& getProcessName() const { return m_strProcessName; }
		const std::string& getUniqueID() const { return m_strUniqueID; }
		ClientID getMegaClientID() const { return m_megaClientID; }
		ClientID getEGClientID() const { return m_egClientID; }
		
		void setEGClientID( ClientID clientID ) { m_egClientID = clientID; }
		
	private:
		std::string m_strProcessName;
		std::string m_strUniqueID;
		ClientID m_megaClientID;
		ClientID m_egClientID;
	};


	class HostMap
	{
	public:
		using HostProcessNameMap = std::multimap< std::string, Host >;
		using HostNameMap = std::map< std::string, Host >;
		using ProcessNameToHostNameMap = std::multimap< std::string, std::string >;
	
		HostMap();
		
		HostMap( const HostMap& oldHostMapping, 
			ProcessNameToHostNameMap processNameToHostNameMap, 
			std::vector< Host >& unmappedClients, 
			std::vector< std::string >& unmappedHostNames );
			
		const HostProcessNameMap& getEnrolledHosts() const { return m_hostProcessNameMap; }
		const HostNameMap& getHostNameMapping() const { return m_hostnameMapping; }
		void removeClient( Host::ClientID uiClient );
		void listClients( std::ostream& os ) const;
		bool enroll( const std::string& strProcessName, Host::ClientID clientID, const std::string& strUniqueID );
		bool enrolleg( Host::ClientID clientID, const std::string& strUniqueID );
	
	private:
		std::multimap< std::string, Host > m_hostProcessNameMap;
		std::map< std::string, Host > m_hostnameMapping;
		
	};
	

}

#endif //HOSTMAP_06_MAY_2020