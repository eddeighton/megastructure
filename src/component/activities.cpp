

#include "activities.hpp"

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
		std::cout << 
			"Enroll response success : " << enroll.success() << "\n" <<
			"host path: " << enroll.hostpath() << "\n" <<
			"program: " << enroll.program() << std::endl;
		m_component.activityComplete( shared_from_this() );
		
		if( enroll.success() )
		{
			
			
		}
		else
		{
			std::cout << "Enroll failed" << std::endl;
		}
		
		
		return true;
	}
	return false;
}
	
	
	
}