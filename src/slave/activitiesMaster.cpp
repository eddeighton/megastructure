
#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"

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
			if( m_slave.getActiveProgramName().empty() )
			{
				std::cout << "No currently active program" << std::endl;
			}
			else
			{
				std::cout << "Active program is: " << m_slave.getActiveProgramName() << std::endl;
			}
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
				std::cout << "Got alive test request. Responded true."  << std::endl;
			}
			else
			{
				pAlive->set_success( false );
				std::cout << "Got alive test request. Responded false."  << std::endl;
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
			m_pLoadHosts = LoadHostsProgramActivity::Ptr( 
				new LoadHostsProgramActivity( m_slave, m_currentlyLoadingProgramName ) );
			m_slave.startActivity( m_pLoadHosts );
		}
		else
		{
			std::cout << "Got request to load program: " << loadProgram.programname() 
				<< " while loading program: " << m_currentlyLoadingProgramName << std::endl;
			m_slave.sendMaster( sms_load( false ) );
		}
			
		return true;
	}
	return false;
}

bool LoadProgramActivity::activityComplete( Activity::Ptr pActivity )
{
	if( m_pLoadHosts == pActivity )
	{
		using namespace megastructure;
		
		std::shared_ptr< LoadHostsProgramActivity > pLoad =
			std::dynamic_pointer_cast< LoadHostsProgramActivity >( pActivity );
		VERIFY_RTE( pLoad );
		
		if( pLoad->Successful() )
		{
			m_slave.setActiveProgramName( m_currentlyLoadingProgramName );
			std::cout << "Active program is now: " << m_currentlyLoadingProgramName << std::endl;
			m_slave.sendMaster( sms_load( true ) );
		}
		else
		{
			m_slave.sendMaster( sms_load( false ) );
			std::cout << "Failed to load program: " << m_currentlyLoadingProgramName << std::endl;
		}
		
		m_currentlyLoadingProgramName.clear();
		
		return true;
	}
			
	return false;
}

}