

#include "hostMap.hpp"

#include "megastructure/log.hpp"

#include <iostream>

namespace
{
	template< typename Container, typename ValueType = typename Container::value_type::second_type >
	void eraseByHostMegaClientID( Container& container, const ValueType& value )
	{
		for( auto i = container.begin(), iEnd = container.end(); i!=iEnd;  )
		{
			if( i->second.getMegaClientID() == value )
				i = container.erase( i );
			else
				++i;
		}
	}
}

namespace slave
{

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	HostMap::HostMap()
	{
	}
		
	HostMap::HostMap( const HostMap& oldHostMapping, 
		ProcessNameToHostNameMap processNameToHostNameMap, 
		std::vector< Host >& unmappedClients, 
		std::vector< std::string >& unmappedHostNames )
	{
		HostProcessNameMap 	oldEnrolledHosts 	= oldHostMapping.getEnrolledHosts();
		HostNameMap 		oldHostNames 		= oldHostMapping.getHostNameMapping();
		
		//just copy the enrolled hosts
		m_hostProcessNameMap = oldEnrolledHosts;
		
		//The goal is to best preserve existing hostname mappings where possible.  The new ProcessNameToHostNameMap 
		//defines the set of pairs of process name host names that the new program is to use.  
		
		//map the remaining new host namespace
		for( ProcessNameToHostNameMap::iterator 
			i = processNameToHostNameMap.begin(),
			iEnd = processNameToHostNameMap.end(); 
			i!=iEnd; )
		{
			const std::string& strProcessName 	= i->first;
			const std::string& strHostName 		= i->second;
			
			bool bReUsedExistingHostName = false;
			//host names should be unique - see if the hostname is in the old list
			HostNameMap::iterator iFind = oldHostNames.find( strHostName );
			if( iFind != oldHostNames.end() )
			{
				HostProcessNameMap::const_iterator 
					iLower = oldEnrolledHosts.lower_bound( strProcessName ),
					iUpper = oldEnrolledHosts.upper_bound( strProcessName );
				//found a match - but it may now have the correct process name so check
				//so iterate through ALL matching process names to see if the host is there
				for( HostProcessNameMap::const_iterator iIterator = iLower; iIterator != iUpper; ++iIterator )
				{
					if( iIterator->second.getMegaClientID() == iFind->second.getMegaClientID() )
					{
						m_hostnameMapping.insert( std::make_pair( strHostName, iIterator->second ) );
						
						i = processNameToHostNameMap.erase( i );
						oldHostNames.erase( iFind );
						oldEnrolledHosts.erase( iIterator );
						
						bReUsedExistingHostName = true;
						break;
					}
				}
			}
			if( !bReUsedExistingHostName )
			{
				++i;
			}
		}
		
		//now simply attempt to use the existing correct process names
		for( ProcessNameToHostNameMap::iterator 
			i = processNameToHostNameMap.begin(),
			iEnd = processNameToHostNameMap.end(); 
			i!=iEnd;  )
		{
			const std::string& strProcessName 	= i->first;
			const std::string& strHostName 		= i->second;
			
			HostProcessNameMap::const_iterator 
					iLower = oldEnrolledHosts.lower_bound( strProcessName ),
					iUpper = oldEnrolledHosts.upper_bound( strProcessName );
			if( iLower != iUpper )
			{
				m_hostnameMapping.insert( std::make_pair( strHostName, iLower->second ) );
				oldEnrolledHosts.erase( iLower );
				i = processNameToHostNameMap.erase( i );
			}
			else
			{
				unmappedHostNames.push_back( strHostName );
				++i;
			}
		}
		
		//finally collate the old enrolled hosts that could not help
		for( HostProcessNameMap::const_iterator 
				i = oldEnrolledHosts.begin(),
				iEnd = oldEnrolledHosts.end(); i!=iEnd; ++i )
		{
			unmappedClients.push_back( i->second );
		}
	}
	
	void HostMap::listClients( std::ostream& os ) const
	{
		for( const auto& i : m_hostProcessNameMap )
		{
			bool bFound = false;
			for( const auto& h : m_hostnameMapping )
			{
				if( h.second.getMegaClientID() == i.second.getMegaClientID() )
				{
                    SPDLOG_INFO( "Client: {} ProcessName: {} HostName: {}", 
                        i.second.getMegaClientID(), i.first, h.first );
					bFound = true;
					break;
				}
			}
			if( !bFound )
			{
                SPDLOG_INFO( "Client: {} ProcessName: {} No Host Name Assigned", 
                    i.second.getMegaClientID(), i.first );
			}
		}
	}
	
	void HostMap::removeClient( std::uint32_t uiClient )
	{
		eraseByHostMegaClientID( m_hostProcessNameMap, uiClient );
		eraseByHostMegaClientID( m_hostnameMapping, uiClient );
	}
	bool HostMap::enroll( const std::string& strProcessName, std::uint32_t clientID, const std::string& strUniqueID )
	{
		m_hostProcessNameMap.insert( std::make_pair( strProcessName, Host( strProcessName, clientID, strUniqueID ) ) );
		return true;
	}
		
	bool HostMap::enrolleg( Host::ClientID clientID, const std::string& strUniqueID )
	{
		for( HostProcessNameMap::iterator 
				i = m_hostProcessNameMap.begin(),
				iEnd = m_hostProcessNameMap.end(); i!=iEnd ; ++i )
		{
			Host& host = i->second;
			
			if( host.getUniqueID() == strUniqueID )
			{
				host.setEGClientID( clientID );
                SPDLOG_INFO( "Host enrolled as process: {} mega client id: {} eg client id: {}",
                    host.getProcessName(), host.getMegaClientID(), host.getEGClientID() );
				return true;
			}
		}
		
		return true;
	}
}