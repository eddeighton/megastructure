
#ifndef ACTIVITIES_MASTER_26_APRIL_2020
#define ACTIVITIES_MASTER_26_APRIL_2020

#include "slave.hpp"

namespace slave
{
	
	class MasterEnrollActivity : public megastructure::Activity
	{
	public:
		MasterEnrollActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual void start();
		virtual bool serverMessage( const megastructure::Message& message );
		
	private:
		Slave& m_slave;
	};

		
	class AliveTestActivity : public megastructure::Activity
	{
	public:
		AliveTestActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual bool serverMessage( const megastructure::Message& message );
		
	private:
		Slave& m_slave;
	};

}

#endif //ACTIVITIES_MASTER_26_APRIL_2020