

#include "activities.hpp"

#include "megastructure/mega.hpp"

#include "jobs.hpp"

#include "protocol/protocol_helpers.hpp"

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


void TestEGReadActivity::start()
{
	Message message;
    {
		Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
		
		pEGMsg->set_type( m_uiType );
		pEGMsg->set_instance( m_uiInstance );
		pEGMsg->set_cycle( 0 );
		
		Message::EG_Msg::Request* pEGRequest = pEGMsg->mutable_request();
		pEGRequest->set_coordinator( 0U );
		pEGRequest->set_host( 0U );
		
		Message::EG_Msg::Request::Read* pEGReadRequest = pEGRequest->mutable_read();
		
    }
	std::cout << "Sending eg read for type: " << m_uiType << " instance: " << m_uiInstance << std::endl;
    m_component.send( message );
}

bool TestEGReadActivity::serverMessage( const Message& message )
{
	if( message.has_eg_msg() )
	{
		const Message::EG_Msg& egMsg = message.eg_msg();
		
		if( egMsg.type() == m_uiType &&
			egMsg.instance() == m_uiInstance &&
			egMsg.cycle() == 0U )
		{
			if( egMsg.has_response() )
			{
				const Message::EG_Msg::Response& readResponse = egMsg.response();
				const std::string& strValue = readResponse.value();
				
				m_resultPromise.set_value( strValue );
				m_component.activityComplete( shared_from_this() );
				
				std::cout << "Get eg read response for type: " << m_uiType << " instance: " << m_uiInstance << " of: " << strValue << std::endl;
			}
			return true;
		}
    }
	return false;
}


void TestEGWriteActivity::start()
{
	Message message;
    {
		Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
		
		pEGMsg->set_type( m_uiType );
		pEGMsg->set_instance( m_uiInstance );
		pEGMsg->set_cycle( 0 );
		
		Message::EG_Msg::Request* pEGRequest = pEGMsg->mutable_request();
		pEGRequest->set_coordinator( 0U );
		pEGRequest->set_host( 0U );
		
		Message::EG_Msg::Request::Write* pEGWrite = pEGRequest->mutable_write();
		pEGWrite->set_value( m_strBuffer );
    }
	std::cout << "Sending eg write for type: " << m_uiType << " instance: " << m_uiInstance << " value: " << m_strBuffer << std::endl;
    m_component.send( message );
}


void TestEGCallActivity::start()
{
	Message message;
    {
		Message::EG_Msg* pEGMsg = message.mutable_eg_msg();
		
		pEGMsg->set_type( m_uiType );
		pEGMsg->set_instance( m_uiInstance );
		pEGMsg->set_cycle( 0 );
		
		Message::EG_Msg::Request* pEGRequest = pEGMsg->mutable_request();
		pEGRequest->set_coordinator( 0U );
		pEGRequest->set_host( 0U );
		
		Message::EG_Msg::Request::Call* pEGCall = pEGRequest->mutable_call();
		pEGCall->set_args( m_strBuffer );
    }
	std::cout << "Sending eg call for type: " << m_uiType << " instance: " << m_uiInstance << " args: " << m_strBuffer << std::endl;
    m_component.send( message );
}


}