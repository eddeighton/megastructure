
#ifndef ACTIVITIES_MASTER_26_APRIL_2020
#define ACTIVITIES_MASTER_26_APRIL_2020

#include "slave.hpp"

#include "megastructure/activity.hpp"

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

	class LoadProgramActivity : public megastructure::ExclusiveActivity< LoadProgramActivity >
	{
	public:
		LoadProgramActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual bool serverMessage( const megastructure::Message& message );
		virtual bool activityComplete( Activity::Ptr pActivity );
		
	private:
		std::string m_currentlyLoadingProgramName;
		Slave& m_slave;
		Activity::Ptr m_pTestHosts;
		Activity::Ptr m_pLoadHosts;
	};

	class ConfigActivity : public megastructure::ExclusiveActivity< ConfigActivity >
	{
	public:
		ConfigActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual bool serverMessage( const megastructure::Message& message );
		virtual bool activityComplete( Activity::Ptr pActivity );
		
	private:
        megastructure::ConfigActivityType m_activityType = 
            megastructure::TOTAL_CONFIG_ACTIVITY_TYPES;
		Slave& m_slave;
		Activity::Ptr m_pTestHosts;
		Activity::Ptr m_pConfigHosts;
	};
}

#endif //ACTIVITIES_MASTER_26_APRIL_2020