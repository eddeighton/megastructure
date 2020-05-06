
#ifndef ACTIVITIES_HOST_26_APRIL_2020
#define ACTIVITIES_HOST_26_APRIL_2020

#include "slave.hpp"

#include "megastructure/activity.hpp"

namespace slave
{
		
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class TestHostActivity : public megastructure::Activity
	{
	public:
		TestHostActivity( Slave& slave, 
							const Host& host, 
							const std::string& strProcessName ) 
			:	m_slave( slave ),
				m_host( host ),
				m_strProcessName( strProcessName ),
				m_bSuccess( false )
		{
			
		}
		
		virtual void start();
		virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
		
		bool isAlive() const
		{
			return m_bSuccess;
		}
		
		const std::string& getName() const
		{
			return m_strProcessName;
		}
		
		std::uint32_t getClientID() const 
		{
			return m_host.getMegaClientID();
		}
	private:
		Slave& m_slave;
		const Host m_host;
		std::string m_strProcessName;
		bool m_bSuccess;
	};

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class TestHostsActivity : public megastructure::Activity
	{
	public:
		TestHostsActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual void start();
		virtual bool activityComplete( Activity::Ptr pActivity );
		
	private:
		Slave& m_slave;
		megastructure::Activity::PtrList m_activities;
	};


	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class ListHostsActivity : public megastructure::Activity
	{
	public:
		ListHostsActivity( Slave& slave ) 
			:	m_slave( slave )
		{
			
		}
		
		virtual void start()
		{
			m_slave.listClients( std::cout );
			m_slave.activityComplete( shared_from_this() );
		}
		
	private:
		Slave& m_slave;
	};

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class HostEnrollActivity : public megastructure::Activity
	{
	public:
		HostEnrollActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
		
		virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
		
	private:
		Slave& m_slave;
		std::map< std::shared_ptr< TestHostActivity >, std::uint32_t > m_testsMap;
	};

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class LoadHostProgramActivity : public megastructure::Activity
	{
	public:
		LoadHostProgramActivity( Slave& slave, const Host& host, 
					const std::string& strHostName, const std::string& strProgramName ) 
			:	m_slave( slave ),
				m_hostName( strHostName ),
				m_programName( strProgramName ),
				m_host( host )
		{
		}
		
		virtual void start();
		virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
		bool Successful() const { return m_bSuccess; }
	private:
		Slave& m_slave;
		std::string m_hostName;
		std::string m_programName;
		Host m_host;
		bool m_bSuccess = true;
	};
	
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class LoadHostsProgramActivity : public megastructure::ExclusiveActivity< LoadHostsProgramActivity >
	{
	public:
		LoadHostsProgramActivity( Slave& slave, const std::string& strProgramName ) 
			:	m_slave( slave ),
				m_programName( strProgramName )
		{
		}
		
		virtual void start();
		virtual bool activityComplete( Activity::Ptr pActivity );
		
		bool Successful() const { return m_bSuccess; }
		const std::string& getNewProgramName() const { return m_programName; }
	private:
		Slave& m_slave;
		std::string m_programName;
		std::set< Activity::Ptr > m_loadActivities;
		bool m_bSuccess = true;
	};
    
    
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
    class HostBufferActivity : public megastructure::ExclusiveActivity< HostBufferActivity >
    {
    public:
		HostBufferActivity( Slave& slave ) 
			:	m_slave( slave )
		{
		}
    
		virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
        
    private:
		Slave& m_slave;
        
    };
}

#endif //ACTIVITIES_HOST_26_APRIL_2020