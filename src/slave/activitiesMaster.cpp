
#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"

#include "megastructure/log.hpp"

#include "protocol/protocol_helpers.hpp"

namespace slave
{

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void MasterEnrollActivity::start()
{
	using namespace megastructure;
	Message message;
	{
		Message::SMQ_Enroll* pEnroll =
			message.mutable_smq_enroll();
		pEnroll->set_slavename( m_slave.getName() );
	}
	m_slave.sendMaster( message );
}
	
bool MasterEnrollActivity::serverMessage( const megastructure::Message& message )
{
	if( message.has_mss_enroll() )
	{
		m_slave.setEnrollment( message.mss_enroll().success() );
		
		if( message.mss_enroll().success() )
		{
			m_slave.setActiveProgramName( message.mss_enroll().programname() );
		}
		
		m_slave.activityComplete( shared_from_this() );
		return true;
	}
	return false;
}

	
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool AliveTestActivity::serverMessage( const megastructure::Message& message )
{
	if( message.has_msq_alive() )
	{
		using namespace megastructure;
		
		const Message::MSQ_Alive& alive = 
			message.msq_alive();
			
		Message response;
		{
			Message::SMS_Alive* pAlive =
				response.mutable_sms_alive();
			
			if( alive.slavename() == m_slave.getName() )
			{
				pAlive->set_success( true );
                SPDLOG_TRACE( "Got alive test request. Responded true." );
			}
			else
			{
				pAlive->set_success( false );
                SPDLOG_TRACE( "Got alive test request. Responded false." );
			}
		}
		
		m_slave.sendMaster( response );
			
		return true;
	}
	return false;
}

	
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

bool LoadProgramActivity::serverMessage( const megastructure::Message& message )
{
	if( message.has_msq_load() )
	{
		using namespace megastructure;
		
		const Message::MSQ_Load& loadProgram = message.msq_load();
		
		if( m_currentlyLoadingProgramName.empty() )
		{
			m_currentlyLoadingProgramName = loadProgram.programname();
			
			//start by issuing a test hosts activity
			m_pTestHosts = TestHostsActivity::Ptr( 
				new TestHostsActivity( m_slave ) );
			
			m_pLoadHosts = LoadHostsProgramActivity::Ptr( 
				new LoadHostsProgramActivity( m_slave, m_currentlyLoadingProgramName ) );
				
			m_slave.startActivity( m_pTestHosts );
		}
		else
		{
            SPDLOG_WARN( "Got request to load program: {} while loading program: {}", 
                loadProgram.programname(), m_currentlyLoadingProgramName );
			m_slave.sendMaster( sms_load( false ) );
		}
			
		return true;
	}
	return false;
}

bool LoadProgramActivity::activityComplete( Activity::Ptr pActivity )
{
	if( m_pTestHosts == pActivity )
	{
		m_slave.startActivity( m_pLoadHosts );
		return true;
	}
	else if( m_pLoadHosts == pActivity )
	{
		using namespace megastructure;
		
		std::shared_ptr< LoadHostsProgramActivity > pLoad =
			std::dynamic_pointer_cast< LoadHostsProgramActivity >( pActivity );
		VERIFY_RTE( pLoad );
		
		if( pLoad->Successful() )
		{
			m_slave.setActiveProgramName( m_currentlyLoadingProgramName );
            SPDLOG_INFO( "Active program is now: {}", m_currentlyLoadingProgramName );
			m_slave.sendMaster( sms_load( true ) );
		}
		else
		{
			m_slave.sendMaster( sms_load( false ) );
            SPDLOG_WARN( "Failed to load program: {}", m_currentlyLoadingProgramName );
		}
		
		m_currentlyLoadingProgramName.clear();
		
		return true;
	}
			
	return false;
}

}