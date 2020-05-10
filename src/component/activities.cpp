

#include "activities.hpp"
#include "jobs.hpp"

#include "megastructure/mega.hpp"

#include "protocol/protocol_helpers.hpp"

#include "common/assert_verify.hpp"

namespace megastructure
{
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

EnrollHostActivity::EnrollHostActivity( 
			Component& component,
			const std::string& hostprogram ) 
	:	m_component( component ),
		m_hostprogram( hostprogram ),
		m_strUnique( generateUniqueString() )
{
}

void EnrollHostActivity::start()
{
	Message message;
	{
		Message::HCQ_Enroll* pEnroll = message.mutable_hcq_enroll();
		pEnroll->set_processname( m_hostprogram );
		pEnroll->set_unique( m_strUnique );
	}
	m_component.send( message );
}

bool EnrollHostActivity::serverMessage( const Message& message )
{
	if( message.has_chs_enroll() )
	{
		const Message::CHS_Enroll& enroll = message.chs_enroll();
		
		if( enroll.success() )
		{
			m_strWorkspace = enroll.workspacepath();
			m_strSlaveName = enroll.slavename();
			
			{
				Message messageNext;
				{
					Message::HCQ_EnrollEG* pEnrolleg = messageNext.mutable_hcq_enrolleg();
					pEnrolleg->set_unique( m_strUnique );
				}
				m_component.sendeg( messageNext );
			}
		}
		else
		{
			std::cout << "Enroll failed" << std::endl;
			m_component.activityComplete( shared_from_this() );
		}
		return true;
	}
	else if( message.has_chs_enrolleg() )
	{
		const Message::CHS_EnrollEG& enrolleg = message.chs_enrolleg();
		if( enrolleg.success() )
		{
			m_component.setSlaveWorkspacePath( m_strWorkspace );
			m_component.setSlaveName( m_strSlaveName );
			std::cout << "Enroll succeeded.  Slave workspace: " << m_strWorkspace << 
				" Slave name: " <<  m_strSlaveName << std::endl;
		}
		else
		{
			std::cout << "Enroll failed" << std::endl;
		}
		m_component.activityComplete( shared_from_this() );
		return true;
	}
	return false;
}
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
bool LoadProgramActivity::serverMessage( const Message& message )
{
	if( message.has_chq_load() )
	{
		const Message::CHQ_Load& loadProgramRequest = message.chq_load();
		
		if( m_pLoadProgramJob )
		{
			//existing job active so deny
			std::cout << "Cannot load program while already loading a program" << std::endl;
			m_component.send( megastructure::hcs_load( false ) );
		}
		else
		{
			m_component.requestSimulationLock( shared_from_this() );
					
			m_strProgramName 	= loadProgramRequest.programname();
			m_strHostName 		= loadProgramRequest.hostname();
            m_pLoadProgramJob.reset( new LoadProgramJob( m_component, 
                m_strHostName, m_strProgramName ) );
            m_component.startJob( m_pLoadProgramJob );
		}
			
		return true;
	}
	return false;
}
	
bool LoadProgramActivity::jobComplete( Job::Ptr pJob )
{
	if( m_pLoadProgramJob == pJob )
	{
        if( m_pLoadProgramJob->successful() )
        {
			std::cout << "Program: " << m_strProgramName << 
				" HostName: " << m_strHostName << 
				" loaded successfully" << std::endl;
            m_component.send( megastructure::hcs_load( true ) );
        }
        else
        {
			std::cout << "Program: " << m_strProgramName << 
				" HostName: " << m_strHostName << 
				" failed to load" << std::endl;
            m_component.send( megastructure::hcs_load( false ) );
        }
		m_pLoadProgramJob.reset();
		m_component.releaseSimulationLock( shared_from_this() );
		return true;
	}
	return false;
}
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void BufferActivity::start()
{
    //std::cout << "Sending buffer request for: " << m_bufferName << " size: " << m_bufferSize << std::endl;
    Message message;
    {
        Message::HCQ_Buffer* pBufferRequest = message.mutable_hcq_buffer();
        pBufferRequest->set_buffername( m_bufferName );
        pBufferRequest->set_size( m_bufferSize );
    }
    m_component.send( message );
}

bool BufferActivity::serverMessage( const Message& message )
{
	if( message.has_chs_buffer() )
	{
		const Message::CHS_Buffer& bufferResponse = message.chs_buffer();
        if( bufferResponse.buffername() == m_bufferName )
        {
            //std::cout << "Got buffer response for: " << 
            //    m_bufferName << " size: " << m_bufferSize << 
            //    " : " << bufferResponse.sharedname() << std::endl;
            m_result.set_value( bufferResponse.sharedname() );
			m_component.activityComplete( shared_from_this() );
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

EGRequestHandlerActivity::EGRequestHandlerActivity( Component& component ) 
	:	m_component( component ),
		m_bHasSimulationLock( false )
{
	
}


void EGRequestHandlerActivity::request( const Message& message )
{
	m_messages.push_back( message );
	
	/*if( m_bHasSimulationLock )
	{
		
	}
	else
	{
		
	}*/
}

void EGRequestHandlerActivity::simulationLockGranted()
{
	m_bHasSimulationLock = true;
	
	processMessages();
	
	m_component.releaseSimulationLock( shared_from_this() );
	m_component.activityComplete( shared_from_this() );
}

void EGRequestHandlerActivity::processMessages()
{
	for( const Message& msg : m_messages )
	{
		const Message::EG_Msg& egMsg = msg.eg_msg();
		const Message::EG_Msg::Request& egRequest = egMsg.request();
		
		if( egRequest.has_read() )
		{
			const std::int32_t iType = egMsg.type();
			const std::uint32_t uiInstance = egMsg.instance();
			
			if( std::shared_ptr< Program > pProgram = m_component.getProgram() )
			{
				std::string strBuffer;
				pProgram->readBuffer( iType, uiInstance, strBuffer );
				
				Message responseMessage;
				{
					Message::EG_Msg* pEGMsg = responseMessage.mutable_eg_msg();
					pEGMsg->set_type( iType );
					pEGMsg->set_instance( uiInstance );
					pEGMsg->set_cycle( 0 );
					
					Message::EG_Msg::Response* pResponse = pEGMsg->mutable_response();
					pResponse->set_coordinator( egRequest.coordinator() );
					pResponse->set_host( egRequest.host() );
					pResponse->set_value( strBuffer );
				}
				std::cout << "Sending eg response type: " << egMsg.type() << " instance: " << egMsg.instance() <<
					" cycle: " << egMsg.cycle() << " coordinator: " << egRequest.coordinator() << 
					" host: " << egRequest.host() << std::endl;
				m_component.sendeg( responseMessage );
			}
			else
			{
				//error
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
				std::cout << "Sending eg error type: " << egMsg.type() << " instance: " << egMsg.instance() <<
					" cycle: " << egMsg.cycle() << " coordinator: " << egRequest.coordinator() << 
					" host: " << egRequest.host() << std::endl;
				m_component.sendeg( errorMessage );
			}
		}
		else if( egRequest.has_write() )
		{
		}
		else if( egRequest.has_call() )
		{
		}
		else if( egRequest.has_lock() )
		{
		}
		else if( egRequest.has_unlock() )
		{
			
			
		}
		else
		{
			THROW_RTE( "Unknown eg message type" );
		}
	}
	m_messages.clear();
	
}
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

bool EGRequestManagerActivity::serverMessage( const Message& message )
{
	if( message.has_eg_msg() )
	{
		const Message::EG_Msg& egMsg = message.eg_msg();
		if( egMsg.has_request() )
		{
			const Message::EG_Msg::Request& egRequest = egMsg.request();
			
			const std::uint32_t uiCoordinator 	= egRequest.coordinator();
			const std::uint32_t uiHost 			= egRequest.host();
			const std::uint32_t uiCycle 		= egMsg.cycle();
			
			const CoordinatorHostCycle chc( uiCoordinator, uiHost, uiCycle );
			
			EGRequestHandlerActivity::Ptr pHandler;
			{
				ActivityMap::const_iterator iFind = m_activities.find( chc );
				if( iFind != m_activities.end() )
				{
					pHandler = iFind->second;
				}
				else
				{
					//create new handler
					pHandler.reset( new EGRequestHandlerActivity( m_component ) );
					m_component.startActivity( pHandler );
					m_component.requestSimulationLock( pHandler );
					m_activities.insert( std::make_pair( chc, pHandler ) );
				}
			}
			
			pHandler->request( message );
		}
		else if( egMsg.has_response() )
		{
			THROW_RTE( "EGRequestManagerActivity received eg response" );
		}
		else if( egMsg.has_error() )
		{
			const Message::EG_Msg::Error& egError = egMsg.error();
			
			std::cout << "Got eg error type: " << egMsg.type() << " instance: " << egMsg.instance() <<
				" cycle: " << egMsg.cycle() << " coordinator: " << egError.coordinator() << 
				" host: " << egError.host() << std::endl;
		}
		else if( egMsg.has_event() )
		{
			THROW_RTE( "EGRequestManagerActivity received eg event" );
		}
		
		return true;
	}
	return false;
}

bool EGRequestManagerActivity::activityComplete( Activity::Ptr pActivity )
{
	for( ActivityMap::iterator i = m_activities.begin(),
		iEnd = m_activities.end(); i!=iEnd; ++i )
	{
		if( i->second == pActivity )
		{
			m_activities.erase( i );
			return true;
		}
	}
	
	return false;
}

}