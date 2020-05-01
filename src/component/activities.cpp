

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
			Message response;
			{
				Message::HCS_Load* pLoadResponse = response.mutable_hcs_load();
				pLoadResponse->set_success( false );
			}
			m_component.send( response );
		}
		else
		{
			Program::Ptr pNewProgram;
		
			if( loadProgramRequest.hostname().empty() )
			{
				std::cout << "Received load program request to unload" << std::endl;
				m_pLoadProgramJob.reset( new LoadProgramJob( m_component, pNewProgram ) );
				m_component.startJob( m_pLoadProgramJob );
			}
			else
			{
				std::cout << "Received load program request for host name: " << 
					loadProgramRequest.hostname() << 
					" program name: " << loadProgramRequest.programname() << std::endl;
					
				try
				{
					pNewProgram.reset( 
						new Program( 
							m_component, 
							loadProgramRequest.hostname(),
							loadProgramRequest.programname() ) );
				}
				catch( std::exception& ex )
				{
					Message response;
					{
						Message::HCS_Load* pLoadResponse = response.mutable_hcs_load();
						pLoadResponse->set_success( false );
					}
					m_component.send( response );
					std::cout << "Encountered error while attempting to load program: " << ex.what() << std::endl;
				}
						
				if( pNewProgram )
				{
					std::cout << "Load program successful so setting as active program" << std::endl;
					m_pLoadProgramJob.reset( new LoadProgramJob( m_component, pNewProgram ) );
					m_component.startJob( m_pLoadProgramJob );
				}
			}
		}
			
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