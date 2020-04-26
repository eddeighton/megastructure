
#include "activitiesHost.hpp"

namespace slave
{
	
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TestHostActivity::start()
{
	using namespace megastructure;
	Message message;
	{
		Message::CHQ_Alive* pAlive = message.mutable_chq_alive();
		pAlive->set_processname( m_strProcessName );
	}
	if( !m_slave.sendHost( message, m_clientID ) )
	{
		m_slave.removeClient( m_clientID );
		m_slave.activityComplete( shared_from_this() );
	}
}

bool TestHostActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( m_clientID == uiClient )
	{
		if( message.has_hcs_alive() )
		{
			const megastructure::Message::HCS_Alive& alive = message.hcs_alive();
			if( !alive.success() )
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is not alive" << std::endl;
				m_slave.removeClient( m_clientID );
			}
			else
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is alive" << std::endl;
				m_bSuccess = true;
			}
			m_slave.activityComplete( shared_from_this() );
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TestHostsActivity::start()
{
	const megastructure::ClientMap::ClientIDMap& clients = m_slave.getClients();
	for( megastructure::ClientMap::ClientIDMap::const_iterator 
		i = clients.begin(), iEnd = clients.end();
		i!=iEnd; ++i )
	{
		megastructure::Activity::Ptr pActivity( 
			new TestHostActivity( m_slave, i->second, i->first ) );
		m_activities.push_back( pActivity );
		m_slave.startActivity( pActivity );
	}
}

bool TestHostsActivity::activityComplete( Activity::Ptr pActivity )
{
	megastructure::Activity::PtrList::iterator iFind = 
		std::find( m_activities.begin(), m_activities.end(), pActivity );
	if( iFind == m_activities.end() )
	{
		return false;
	}
	else
	{
		m_activities.erase( iFind );
		if( m_activities.empty() )
		{
			m_slave.activityComplete( shared_from_this() );
		}
		return true;
	}
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool HostEnrollActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	using namespace megastructure;
	
	if( message.has_hcq_enroll() )
	{
		const Message::HCQ_Enroll& enroll = message.hcq_enroll();
		
		std::cout << "Enroll request from: " << uiClient << 
			" for role: " << enroll.processname() << std::endl;
			
		if( m_slave.enroll( enroll.processname(), uiClient ) )
		{
			if( !m_slave.sendHost( chs_enroll( true, "", "" ), uiClient ) )
			{
				m_slave.removeClient( uiClient );
			}
		}
		else 
		{
			std::uint32_t uiExisting;
			if( m_slave.getClientID( enroll.processname(), uiExisting ) )
			{
				std::cout << "Enroll attempting for: " << enroll.processname() << " which has existing client of: " << uiExisting << std::endl;
				std::shared_ptr< TestHostActivity > pTest = 
					std::make_shared< TestHostActivity >( m_slave, uiExisting, enroll.processname() );
				m_testsMap.insert( std::make_pair( pTest, uiClient ) );
				m_slave.startActivity( pTest );
				return true;
			}
			else
			{
				std::cout << "Enroll denied for: " << enroll.processname() << " for client: " << uiClient << std::endl;
				if( !m_slave.sendHost( chs_enroll( false ), uiClient ) )
				{
					m_slave.removeClient( uiClient );
				}
			}
		}
		
		return true;
	}
	return false;
}

bool HostEnrollActivity::activityComplete( Activity::Ptr pActivity )
{
	using namespace megastructure;
	if( std::shared_ptr< TestHostActivity > pTest = 
			std::dynamic_pointer_cast< TestHostActivity >( pActivity ) )
	{
		std::map< std::shared_ptr< TestHostActivity >, std::uint32_t >::iterator 
			iFind = m_testsMap.find( pTest );
		if( iFind != m_testsMap.end() )
		{
			const std::uint32_t testedID = pTest->getClientID();
			const std::uint32_t clientID = iFind->second;
			m_testsMap.erase( iFind );
			if( pTest->isAlive() )
			{
				std::cout << "Existing client: " << clientID << " is alive as: " << pTest->getName() << std::endl;
				//existing client is alive so nothing we can do...
				if( !m_slave.sendHost( chs_enroll( false ), clientID ) )
				{
					m_slave.removeClient( clientID );
				}
			}
			else
			{
				std::cout << "Existing client: " << testedID << " is not alive so allowing enrollment of new client: " << 
					clientID << " as: " << pTest->getName() << std::endl;
				//testing the existing client indicated it was actually dead so can enroll the new one
				if( m_slave.enroll( pTest->getName(), clientID ) )
				{
					if( !m_slave.sendHost( chs_enroll( true, "/someplace", "AProgramFolderName" ), clientID ) )
					{
						m_slave.removeClient( clientID );
					}
				}
				else
				{
					std::cout << "Enroll denied after retry for: " << pTest->getName() << " for client: " << clientID << std::endl;
					m_slave.sendHost( chs_enroll( false ), clientID );
				}
			}
			return true;
		}
	}
	return false;
}

}