
#include "activitiesEG.hpp"

namespace slave
{

bool RouteEGProtocolActivity::serverMessage( const megastructure::Message& message )
{
	return false;
}
		
bool RouteEGProtocolActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( message.has_eg_msg() )
	{
		
		//determine how to route the request based on the type
		/*
		if( megastructure::NetworkAddressTable::Ptr pNAT = m_slave.getNetworkAddressTable() )
		{
			const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
			
			if( egMsg.has_egs_read() ) //read response
			{
				//route back to where the request came from
				const megastructure::Message::EG_Msg::EGS_Read& egsRead = egMsg.egs_read();
				
				if( egsRead.coordinator() != 0U )
				{
					std::cout << "Slave: " << m_slave.getName() << " routine eg read response to master" << std::endl;
					m_slave.sendMaster( message );
				}
				else if( egsRead.host() != 0U ) 
				{
					std::cout << "Slave: " << m_slave.getName() << " routine eg read response to host: " << egsRead.host() << std::endl;
					m_slave.sendHost( message, egsRead.host() );
				}
				else
				{
					THROW_RTE( "EGS Read has not host return address specified" );
				}
			}
			else if( egMsg.has_eg_event() ) //event
			{
				//forward to everything
				m_slave.sendMaster( message );
				
				const auto& hosts = m_slave.getHosts().getEnrolledHosts();
				for( auto i : hosts )
				{
					if( i.second != uiClient )
					{
						m_slave.sendHost( message, i.second );
					}
				}
			}
			else //requests
			{
				const std::uint32_t uiType = egMsg.type();
				const std::uint32_t uiTargetClientID = pNAT->getClientForType( uiType );
				VERIFY_RTE( uiTargetClientID != uiClient );
				
				if( uiTargetClientID == megastructure::NetworkAddressTable::MasterID )
				{
					if( egMsg.has_egq_read() ) //request
					{
						megastructure::Message msgCopy = message;
						megastructure::Message::EG_Msg* pEGMsg = msgCopy.mutable_eg_msg();
						megastructure::Message::EG_Msg::EGQ_Read* pEGQRead = pEGMsg->mutable_egq_read();
						
						pEGQRead->set_host( uiClient );
						
						m_slave.sendMaster( msgCopy );
					}
					else
					{
						//forward message to master unchanged
						m_slave.sendMaster( message );
					}
				}
				else
				{
					if( egMsg.has_egq_read() ) //request
					{
						megastructure::Message msgCopy = message;
						megastructure::Message::EG_Msg* pEGMsg = msgCopy.mutable_eg_msg();
						megastructure::Message::EG_Msg::EGQ_Read* pEGQRead = pEGMsg->mutable_egq_read();
						
						pEGQRead->set_host( uiClient );
						
						//forward message to client
						m_slave.sendHost( msgCopy, uiTargetClientID );
					}
					else
					{
						//forward message to client
						m_slave.sendHost( message, uiTargetClientID );
					}
				}
			}
		}
		else
		{
			std::cout << "EG Msg received when no network address table configured" << std::endl;
		}
		*/
		return true;
	}
	
	return false;
}
		
}