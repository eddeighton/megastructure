

#include "activities.hpp"

#include "jobs.hpp"

#include "protocol/protocol_helpers.hpp"

namespace megastructure
{
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void EnrollHostActivity::start()
{
	std::cout << "EnrollActivity started" << std::endl;
	
	Message message;
	{
		Message::HCQ_Enroll* pEnroll = message.mutable_hcq_enroll();
		pEnroll->set_processname( m_hostprogram );
	}
	m_component.send( message );
}

bool EnrollHostActivity::serverMessage( const Message& message )
{
	if( message.has_chs_enroll() )
	{
		const Message::CHS_Enroll& enroll = message.chs_enroll();
		m_component.activityComplete( shared_from_this() );
		
		if( enroll.success() )
		{
			m_component.setSlaveWorkspacePath( enroll.workspacepath() );
			m_component.setSlaveName( enroll.slavename() );
			std::cout << "Enroll succeeded.  Slave workspace: " << enroll.workspacepath() << 
				" Slave name: " <<  enroll.slavename() << std::endl;
		}
		else
		{
			std::cout << "Enroll failed" << std::endl;
		}
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
            m_pLoadProgramJob.reset( new LoadProgramJob( m_component, 
                loadProgramRequest.hostname(), 
                loadProgramRequest.programname() ) );
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
            std::cout << "Load program was successful" << std::endl;
            m_component.send( megastructure::hcs_load( true ) );
        }
        else
        {
            std::cout << "Load program was unsuccessful" << std::endl;
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

}