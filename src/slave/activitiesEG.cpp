
#include "activitiesEG.hpp"

#include "megastructure/log.hpp"

namespace slave
{
	
inline megastructure::Message copyRequestSetSourceHost( 
	const megastructure::Message& message, std::uint32_t uiClient )
{
	megastructure::Message msgCopy = message;
	megastructure::Message::EG_Msg* pEGMsg = msgCopy.mutable_eg_msg();
	megastructure::Message::EG_Msg::Request* pRequest = pEGMsg->mutable_request();
	pRequest->set_host( uiClient );
	return msgCopy;
}
	

bool RouteEGProtocolActivity::serverMessage( const megastructure::Message& message )
{
	if( message.has_eg_msg() )
	{
		const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
		
		if( egMsg.has_request() )
		{
			//route the request to the client
			if( megastructure::NetworkAddressTable::Ptr pNAT = m_slave.getNATRequests() )
			{
				const std::uint32_t uiTargetClientID = pNAT->getClientForType( egMsg.type() );
				
				if( uiTargetClientID == megastructure::NetworkAddressTable::MasterID )
				{
                    SPDLOG_ERROR( "Request from master mapped to master id in slave" );
					THROW_RTE( "Request from master mapped to master id in slave" );
				}
				else if( uiTargetClientID == megastructure::NetworkAddressTable::SelfID )
				{
                    SPDLOG_ERROR( "Request from master mapped to self id in slave" );
					THROW_RTE( "Request from master mapped to self id in slave" );
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
						pErrorEGMsgError->set_coordinator( egRequest.coordinator() );
						pErrorEGMsgError->set_host( egRequest.host() );
					}
					
                    SPDLOG_WARN( "Returning error to master due to unmapped target client" );
					m_slave.sendMaster( errorMessage );
				}
				else
				{
                    SPDLOG_TRACE( "Sending request to: {}", uiTargetClientID );
					m_slave.sendHost( message, uiTargetClientID );
				}
			}
			else
			{
                SPDLOG_WARN( "EG request from master received when no network address table configured" );
			}
		}
		else if( egMsg.has_response() )
		{
			//route the response back to the source host
			const megastructure::Message::EG_Msg::Response& response = egMsg.response();
			VERIFY_RTE_MSG( response.host() != 0U, "Invalid host resolved for response from master to slave" );
            SPDLOG_TRACE( "Sending response to: {}", response.host() );
			m_slave.sendHostEG( message, response.host() );
		}
		else if( egMsg.has_error() )
		{
			const megastructure::Message::EG_Msg::Error& error = egMsg.error();
			VERIFY_RTE_MSG( error.host() != 0U, "Invalid host resolved for error from master to slave" );
            SPDLOG_TRACE( "Sending error to: {}", error.host() );
			m_slave.sendHostEG( message, error.host() );
		}
		else if( egMsg.has_event() )
		{
			//route event to all hosts
			const auto& hosts = m_slave.getHosts().getEnrolledHosts();
			for( const auto& i : hosts )
			{
				m_slave.sendHostEG( message, i.second.getEGClientID() );
			}
		}
		
		return true;
	}
	
	return false;
}
		
bool RouteEGProtocolActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( message.has_eg_msg() )
	{
		const megastructure::Message::EG_Msg& egMsg = message.eg_msg();
			
		if( egMsg.has_request() )
		{
			if( megastructure::NetworkAddressTable::Ptr pNAT = m_slave.getNATRequests() )
			{
				//determine whether to route the request to the master or to a host				
				const std::uint32_t uiTargetClientID = pNAT->getClientForType( egMsg.type() );
				VERIFY_RTE( uiTargetClientID != uiClient );
				
				//request is made on the eg socket and the response should be returned there
				const megastructure::Message msgCopy = 
					copyRequestSetSourceHost( message, uiClient );
				
				if( uiTargetClientID == megastructure::NetworkAddressTable::MasterID )
				{
                    SPDLOG_TRACE( "Forwarding request to master" );
					m_slave.sendMaster( msgCopy );
				}
				else if( uiTargetClientID == megastructure::NetworkAddressTable::SelfID )
				{
                    SPDLOG_ERROR( "Request from host mapped to self id in slave" );
					THROW_RTE( "Request from host mapped to self id in slave" );
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
						pErrorEGMsgError->set_coordinator( egRequest.coordinator() );
						pErrorEGMsgError->set_host( egRequest.host() );
					}
					
                    SPDLOG_TRACE( "Forwarding error to host: {}", uiClient );
					m_slave.sendHostEG( errorMessage, uiClient );
				}
				else
				{
                    SPDLOG_TRACE( "Forwarding request to host: {}", uiTargetClientID );
					m_slave.sendHost( msgCopy, uiTargetClientID );
				}
			}
			else
			{
                SPDLOG_WARN( "EG request from client received when no network address table configured" );
			}
		}
		else if( egMsg.has_response() )
		{
			//response needs to be routed back to the source from the client.
			const megastructure::Message::EG_Msg::Response& response = egMsg.response();
			
			//if the response has coordinator then route to master
			if( response.coordinator() != 0U )
			{
                SPDLOG_TRACE( "Forwarding response to master" );
				m_slave.sendMaster( message );
			}
			else
			{
				VERIFY_RTE_MSG( response.host() != 0U, "Response has no source host" );
                SPDLOG_TRACE( "Forwarding response to host: {}", response.host() );
				m_slave.sendHostEG( message, response.host() );
			}
		}
		else if( egMsg.has_error() )
		{
			//response needs to be routed back to the source from the client.
			const megastructure::Message::EG_Msg::Error& error = egMsg.error();
			
			//if the response has coordinator then route to master
			if( error.coordinator() != 0U )
			{
                SPDLOG_TRACE( "Forwarding error to master" );
				m_slave.sendMaster( message );
			}
			else
			{
				VERIFY_RTE_MSG( error.host() != 0U, "Error has no source host" );
                SPDLOG_TRACE( "Forwarding error to host: {}", error.host() );
				m_slave.sendHostEG( message, error.host() );
			}
		}
		else if( egMsg.has_event() )
		{
			//forward to everything except the source
			m_slave.sendMaster( message );
			
			const auto& hosts = m_slave.getHosts().getEnrolledHosts();
			for( const auto& i : hosts )
			{
				if( i.second.getEGClientID() != uiClient )
				{
					m_slave.sendHostEG( message, i.second.getEGClientID() );
				}
			}
		}
		else
		{
			THROW_RTE( "Unknown eg msg type" );
		}
		return true;
	}
	return false;
}
		
}