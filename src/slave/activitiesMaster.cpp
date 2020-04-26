
#include "activitiesMaster.hpp"

namespace slave
{

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void MasterEnrollActivity::start()
{
	std::cout << "MasterEnrollActivity started" << std::endl;
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
		std::cout << "MasterEnrollActivity Got response: " << 
			message.mss_enroll().success() << std::endl;
			
		if( message.mss_enroll().success() )
		{
			
		}
		else
		{
			
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

}