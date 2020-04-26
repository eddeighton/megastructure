

#include "activities.hpp"

namespace master
{

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void TestClientActivity::start()
{
	using namespace megastructure;
	Message message;
	{
		Message::MSQ_Alive* pAlive = message.mutable_msq_alive();
		pAlive->set_slavename( m_strSlaveName );
	}
	if( !m_master.send( message, m_clientID ) )
	{
		m_master.removeClient( m_clientID );
		m_master.activityComplete( shared_from_this() );
	}
}

bool TestClientActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( m_clientID == uiClient )
	{
		if( message.has_sms_alive() )
		{
			const megastructure::Message::SMS_Alive& alive =
				message.sms_alive();
			if( !alive.success() )
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is not alive" << std::endl;
				m_master.removeClient( m_clientID );
			}
			else
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is alive" << std::endl;
				m_bSuccess = true;
			}
			m_master.activityComplete( shared_from_this() );
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void TestClientsActivity::start()
{
	const megastructure::ClientMap::ClientIDMap& clients = m_master.getClients();
	for( megastructure::ClientMap::ClientIDMap::const_iterator 
		i = clients.begin(), iEnd = clients.end();
		i!=iEnd; ++i )
	{
		megastructure::Activity::Ptr pActivity( 
			new TestClientActivity( m_master, i->second, i->first ) );
		m_activities.push_back( pActivity );
		m_master.startActivity( pActivity );
	}
}

bool TestClientsActivity::activityComplete( Activity::Ptr pActivity )
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
			m_master.activityComplete( shared_from_this() );
		}
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
bool EnrollActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	using namespace megastructure;
	
	if( message.has_smq_enroll() )
	{
		const Message::SMQ_Enroll& enroll =
			message.smq_enroll();
		
		std::cout << "Enroll request from: " << uiClient << 
			" for role: " << enroll.slavename() << std::endl;
			
		if( m_master.enroll( enroll.slavename(), uiClient ) )
		{
			if( !m_master.send( mss_enroll( true ), uiClient ) )
			{
				m_master.removeClient( uiClient );
			}
		}
		else 
		{
			std::uint32_t uiExisting;
			if( m_master.getClientID( enroll.slavename(), uiExisting ) )
			{
				std::cout << "Enroll attempting for: " << enroll.slavename() << " which has existing client of: " << uiExisting << std::endl;
				std::shared_ptr< TestClientActivity > pTest = 
					std::make_shared< TestClientActivity >( m_master, uiExisting, enroll.slavename() );
				m_testsMap.insert( std::make_pair( pTest, uiClient ) );
				m_master.startActivity( pTest );
				return true;
			}
			else
			{
				std::cout << "Enroll denied for: " << enroll.slavename() << " for client: " << uiClient << std::endl;
				if( !m_master.send( mss_enroll( false ), uiClient ) )
				{
					m_master.removeClient( uiClient );
				}
			}
		}
		
		return true;
	}
	return false;
}

bool EnrollActivity::activityComplete( Activity::Ptr pActivity )
{
	using namespace megastructure;
	if( std::shared_ptr< TestClientActivity > pTest = 
			std::dynamic_pointer_cast< TestClientActivity >( pActivity ) )
	{
		std::map< std::shared_ptr< TestClientActivity >, std::uint32_t >::iterator 
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
				if( !m_master.send( mss_enroll( false ), clientID ) )
				{
					m_master.removeClient( clientID );
				}
			}
			else
			{
				std::cout << "Existing client: " << testedID << " is not alive so allowing enrollment of new client: " << 
					clientID << " as: " << pTest->getName() << std::endl;
				//testing the existing client indicated it was actually dead so can enroll the new one
				if( m_master.enroll( pTest->getName(), clientID ) )
				{
					if( !m_master.send( mss_enroll( true ), clientID ) )
					{
						m_master.removeClient( clientID );
					}
				}
				else
				{
					std::cout << "Enroll denied after retry for: " << pTest->getName() << " for client: " << clientID << std::endl;
					if( !m_master.send( mss_enroll( false ), clientID ) )
					{
						m_master.removeClient( clientID );
					}
				}
			}
			return true;
		}
	}
	return false;
}	

} //namespace master