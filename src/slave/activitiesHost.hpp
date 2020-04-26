
#ifndef ACTIVITIES_HOST_26_APRIL_2020
#define ACTIVITIES_HOST_26_APRIL_2020

#include "slave.hpp"

namespace slave
{
		
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	class TestHostActivity : public megastructure::Activity
	{
	public:
		TestHostActivity( Slave& slave, 
							std::uint32_t clientID, 
							const std::string& strProcessName ) 
			:	m_slave( slave ),
				m_clientID( clientID ),
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
			return m_clientID;
		}
	private:
		Slave& m_slave;
		std::uint32_t m_clientID;
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
		virtual bool activityComplete( Activity::Ptr pActivity );
		
	private:
		Slave& m_slave;
		std::map< std::shared_ptr< TestHostActivity >, std::uint32_t > m_testsMap;
	};

}

#endif //ACTIVITIES_HOST_26_APRIL_2020