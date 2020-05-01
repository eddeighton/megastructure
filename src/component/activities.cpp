

#include "activities.hpp"

#include "jobs.hpp"

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
			m_component.setSlaveWorkspacePath( enroll.hostpath() );
			std::cout << "Enroll succeeded.  Slave workspace: " << enroll.hostpath() << std::endl;
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
		
		if( loadProgramRequest.hostname().empty() )
		{
			std::cout << "Received load program request to unload" << std::endl;
		}
		else
		{
			std::cout << "Received load program request for host name: " << 
				loadProgramRequest.hostname() << 
				" program name: " << loadProgramRequest.programname() << std::endl;
		}
		
		m_pLoadProgramJob.reset( new LoadProgramJob( 
			m_component,
			loadProgramRequest.hostname(),
			loadProgramRequest.programname() ) );
			
		m_component.startJob( m_pLoadProgramJob );
			
		return true;
	}
	return false;
}
	
bool LoadProgramActivity::jobComplete( Job::Ptr pJob )
{
	if( m_pLoadProgramJob == pJob )
	{
		m_pLoadProgramJob.reset();
		Message response;
		{
			Message::HCS_Load* pLoadResponse = response.mutable_hcs_load();
			pLoadResponse->set_success( true );
		}
		m_component.send( response );
		return true;
	}
	return false;
}
	
}