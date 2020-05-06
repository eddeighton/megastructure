
#ifndef ACTIVITIES_EG_02_MAY_2020
#define ACTIVITIES_EG_02_MAY_2020

#include "slave.hpp"

#include "megastructure/activity.hpp"

namespace slave
{



	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class RouteEGProtocolActivity : public megastructure::Activity
	{
	public:
		RouteEGProtocolActivity( Slave& slave ) 
			:	m_slave( slave )
		{
			
		}
		
		
		virtual bool serverMessage( const megastructure::Message& message );
		virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
		
	private:
		Slave& m_slave;
	};


}

#endif //ACTIVITIES_EG_02_MAY_2020