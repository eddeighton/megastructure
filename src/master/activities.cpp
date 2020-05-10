

#include "activities.hpp"

#include "schema/projectTree.hpp"

#include "common/assert_verify.hpp"

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
	if( m_activities.empty() )
	{
		m_master.activityComplete( shared_from_this() );
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
			if( !m_master.send( mss_enroll( true, m_master.getActiveProgramName() ), uiClient ) )
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
				std::cout << "Existing client: " << clientID << " is alive as: " << pTest->getName() << 
					" so denying enroll request from new client: " << clientID << std::endl;
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
					if( !m_master.send( mss_enroll( true, m_master.getActiveProgramName() ), clientID ) )
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

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

void LoadProgram::start()
{
	//send all clients request to load the program
	const megastructure::ClientMap::ClientIDMap& clients = m_master.getClients();
	if( clients.empty() )
	{
		std::cout << "Master currently has no slaves so cannot load program" << std::endl;
		m_master.activityComplete( shared_from_this() );
	}
	else
	{
		for( megastructure::ClientMap::ClientIDMap::const_iterator 
			i = clients.begin(), iEnd = clients.end();
			i!=iEnd; ++i )
		{
			using namespace megastructure;
			Message message;
			{
				Message::MSQ_Load* pLoad = message.mutable_msq_load();
				pLoad->set_programname( m_strProgramName );
			}
			if( !m_master.send( message, i->second ) )
			{
				m_master.removeClient( i->second );
			}
			else
			{
				m_clientIDs.insert( i->second );
			}
		}
	}
}

bool LoadProgram::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	using namespace megastructure;
	
	if( message.has_sms_load() )
	{
		const Message::SMS_Load& loadProgramResponse = message.sms_load();
		if( !loadProgramResponse.success() )
		{
			std::cout << "Client: " << uiClient << " failed to load program: " << m_strProgramName << std::endl;
			m_clientFailed = true;
		}
		
		m_clientIDs.erase( uiClient );
		
		if( m_clientIDs.empty() )
		{
			if( !m_clientFailed )
			{
				std::cout << "No clients failed while loading program: " << m_strProgramName << std::endl;
			}
			
			try
			{
				std::shared_ptr< ProjectTree > pProjectTree = 
					std::make_shared< ProjectTree >( 
						m_master.getEnvironment(), 
                        m_master.getWorkspace(), 
                        m_strProgramName );
							
				m_master.setActiveProgramName( m_strProgramName );
					
				m_master.calculateNetworkAddressTable( pProjectTree );
			}
			catch( std::exception& ex )
			{
				std::cout << "Error attempting to load project tree for: " << 
					m_strProgramName << " : " << ex.what() << std::endl;
			}
			
			m_master.activityComplete( shared_from_this() );
		}
		
		return true;
	}
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

inline megastructure::Message copyRequestSetSourceCoordinator( 
	const megastructure::Message& message, std::uint32_t uiClient )
{
	megastructure::Message msgCopy = message;
	megastructure::Message::EG_Msg* pEGMsg = msgCopy.mutable_eg_msg();
	megastructure::Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
	pRequest->set_coordinator( uiClient );
	return msgCopy;
}
	
	
bool RouteEGProtocolActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( message.has_eg_msg() )
	{
		const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
		
		if( egMsg.has_request() )
		{
			//route the request to the client
			if( megastructure::NetworkAddressTable::Ptr pNAT = m_master.getNetworkAddressTable() )
			{
				const std::uint32_t uiTargetClientID = pNAT->getClientForType( egMsg.type() );
				if( uiTargetClientID == megastructure::NetworkAddressTable::MasterID )
				{
					THROW_RTE( "Request mapped to master id in master" );
				}
				else if( uiTargetClientID == megastructure::NetworkAddressTable::SelfID )
				{
					THROW_RTE( "Request mapped to self id in master" );
				}
				else if( uiTargetClientID == megastructure::NetworkAddressTable::UnMapped )
				{
					//generate error response that cannot route message
					const megastructure::Message::EG_Msg::Request& egRequest = egMsg.request();
					
					megastructure::Message errorMessage;
					{
						megastructure::Message::EG_Msg* pErrorEGMsg = errorMessage.mutable_eg_msg();
						pErrorEGMsg->set_type( egMsg.type() );
						pErrorEGMsg->set_instance( egMsg.instance() );
						pErrorEGMsg->set_cycle( egMsg.cycle() );
						
						megastructure::Message::EG_Msg::Error* pErrorEGMsgError = pErrorEGMsg->mutable_error();
						pErrorEGMsgError->set_host( egRequest.host() );
					}
					m_master.send( errorMessage, uiClient );
				}
				else
				{
					VERIFY_RTE_MSG( uiTargetClientID != uiClient, "Incorrect routine for eg request" );
					m_master.send( copyRequestSetSourceCoordinator( message, uiClient ), uiTargetClientID );
				}
			}
			else
			{
				std::cout << "EG request from master received when no network address table configured" << std::endl;
			}
		}
		else if( egMsg.has_response() )
		{
			//route the response back to the source host
			const megastructure::Message::EG_Msg::Response& response = egMsg.response();
			VERIFY_RTE_MSG( response.coordinator() != 0U, "Invalid coordinator resolved for response from slave to master" );
			m_master.send( message, response.coordinator() );
		}
		else if( egMsg.has_error() )
		{
			const megastructure::Message::EG_Msg::Error& error = egMsg.error();
			VERIFY_RTE_MSG( error.coordinator() != 0U, "Invalid coordinator resolved for error from slave to master" );
			m_master.send( message, error.coordinator() );
		}
		else if( egMsg.has_event() )
		{
			//route event to all hosts
			const auto& hosts = m_master.getClients();
			for( const auto& i : hosts )
			{
				if( i.second != uiClient )
				{
					m_master.send( message, i.second );
				}
			}
		}
		
		return true;
	}
	
	return false;
}
	
} //namespace master