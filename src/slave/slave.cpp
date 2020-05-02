
#include "slave.hpp"

#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include "boost/filesystem.hpp"
#include "boost/uuid/uuid.hpp"            
#include "boost/uuid/uuid_generators.hpp" 
#include "boost/uuid/uuid_io.hpp"         
#include "boost/lexical_cast.hpp"        

namespace slave
{
    
    inline std::string generateSharedBufferUniqueName()
    {
        const boost::uuids::uuid uuid = boost::uuids::random_generator()();
        return boost::lexical_cast< std::string >( uuid );
    }
    
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	HostMap::HostMap()
	{
	}
		
	HostMap::HostMap( const HostMap& oldHostMapping, 
		ProcessNameToHostNameMap processNameToHostNameMap, 
		std::vector< std::uint32_t >& unmappedClients, 
		std::vector< std::string >& unmappedHostNames )
	{
		HostProcessNameMap 	oldEnrolledHosts 	= oldHostMapping.getEnrolledHosts();
		HostNameMap 		oldHostNames 		= oldHostMapping.getHostNameMapping();
		
		//just copy the enrolled hosts
		m_clientProcessNameMap = oldEnrolledHosts;
		
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
					const std::uint32_t uiClientID = iIterator->second;
					if( uiClientID == iFind->second )
					{
						m_hostnameMapping.insert( std::make_pair( strHostName, uiClientID ) );
						
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
				const std::uint32_t uiClientID = iLower->second;
				
				m_hostnameMapping.insert( std::make_pair( strHostName, uiClientID ) );
				
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
		for( const auto& i : m_clientProcessNameMap )
		{
			bool bFound = false;
			for( const auto& h : m_hostnameMapping )
			{
				if( h.second == i.second )
				{
					std::cout << "Client: " << i.second << " ProcessName: " << i.first << " HostName: " << h.first << std::endl;
					bFound = true;
					break;
				}
			}
			if( !bFound )
			{
				std::cout << "Client: " << i.second << " ProcessName: " << i.first << " HostName: No Host Name Assigned" << std::endl;
			}
		}
	}
	
	void HostMap::removeClient( std::uint32_t uiClient )
	{
		eraseByMapSecond( m_clientProcessNameMap, uiClient );
		eraseByMapSecond( m_hostnameMapping, uiClient );
	}
	bool HostMap::enroll( const std::string& strProcessName, std::uint32_t clientID )
	{
		m_clientProcessNameMap.insert( std::make_pair( strProcessName, clientID ) );
		return true;
	}
		
	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	void getWorkspaceAndSlaveNameFromPath( const boost::filesystem::path& thePath, 
		boost::filesystem::path& workspace, std::string& strSlaveName )
	{
		const boost::filesystem::path canonPath = 
			boost::filesystem::edsCannonicalise(
				boost::filesystem::absolute( thePath ) );
		
		VERIFY_RTE_MSG( boost::filesystem::exists( canonPath ), 
			"Specified slave path does not exist: " << thePath.string() );
		
		workspace = canonPath.parent_path();
		strSlaveName = canonPath.stem().string();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////
	Slave::Slave( 	Environment& environment,
					const std::string& strMasterIP, 
					const std::string strMasterPort, 
					const std::string& strSlavePort, 
					const boost::filesystem::path& strSlavePath )
		:	m_environment( environment ),
			m_server( strSlavePort ),
			m_client( strMasterIP, strMasterPort )
	{
		getWorkspaceAndSlaveNameFromPath( strSlavePath, m_workspacePath, m_strSlaveName );
		
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
		megastructure::Client& client = m_client;
	
		m_zeromqClient = std::thread(
			[ &client, &queue ]()
			{
				megastructure::readClient( client, queue );
			});
			
		m_zeromqServer = std::thread(
			[ &server, &queue ]()
			{
				megastructure::readServer( server, queue );
			});
		
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new MasterEnrollActivity( *this ) );
		m_queue.startActivity( new HostEnrollActivity( *this ) );
		m_queue.startActivity( new LoadProgramActivity( *this ) );
		m_queue.startActivity( new HostBufferActivity( *this ) );
	}
	
	Slave::~Slave()
	{
		m_queue.stop();
		m_server.stop();
		m_client.stop();
		m_zeromqClient.join();
		m_zeromqServer.join();
	}
	
    std::string Slave::getSharedBufferName( const std::string& strBufferName, std::size_t szSize )
    {
        SharedBufferMap::iterator iFind = m_sharedBuffers.find( strBufferName );
        if( iFind != m_sharedBuffers.end() )
        {
            megastructure::SharedBufferImpl::Ptr pBuffer = iFind->second;
            
            //attempt to reuse existing buffer
            if( pBuffer->getSize() == szSize )
            {
                return pBuffer->getSharedName();
            }
        }
        
        const std::string strSharedBufferName = generateSharedBufferUniqueName();
        
        m_sharedBuffers[ strBufferName ] = 
            std::make_shared< megastructure::SharedBufferImpl >( strBufferName, strSharedBufferName, szSize );
        std::cout << "Created shared buffer: " << strBufferName << " size: " << szSize << " " << strSharedBufferName << std::endl;
        
        return strSharedBufferName;
    }
	
}