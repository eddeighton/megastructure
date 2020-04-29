
#ifndef ACTIVITIES_26_APRIL_2020
#define ACTIVITIES_26_APRIL_2020

#include "master.hpp"

#include "megastructure/activity.hpp"

namespace master
{
	

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
class TestClientActivity : public megastructure::Activity
{
public:
	TestClientActivity( Master& master, std::uint32_t clientID, const std::string& strSlaveName ) 
		:	m_master( master ),
			m_clientID( clientID ),
			m_strSlaveName( strSlaveName ),
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
		return m_strSlaveName;
	}
	
	std::uint32_t getClientID() const 
	{
		return m_clientID;
	}
	
private:
	Master& m_master;
	std::uint32_t m_clientID;
	std::string m_strSlaveName;
	bool m_bSuccess;
};

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
class TestClientsActivity : public megastructure::Activity
{
public:
	TestClientsActivity( Master& master ) 
		:	m_master( master )
	{
	}
	
	virtual void start();
	virtual bool activityComplete( Activity::Ptr pActivity );
	
private:
	Master& m_master;
	megastructure::Activity::PtrList m_activities;
};

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
class ListClientsActivity : public megastructure::Activity
{
public:
	ListClientsActivity( Master& master ) 
		:	m_master( master )
	{
	}
	
	virtual void start()
	{
		std::cout << "Listing all clients..." << std::endl;
		const megastructure::ClientMap::ClientIDMap& clients = m_master.getClients();
		for( megastructure::ClientMap::ClientIDMap::const_iterator 
			i = clients.begin(), iEnd = clients.end();
			i!=iEnd; ++i )
		{
			std::cout << "Client: " << i->first << " id: " << i->second << std::endl;
		}
		m_master.activityComplete( shared_from_this() );
	}
	
private:
	Master& m_master;
};

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
class EnrollActivity : public megastructure::ExclusiveActivity< EnrollActivity >
{
public:
	EnrollActivity( Master& master ) 
		:	m_master( master )
	{
	}
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
	virtual bool activityComplete( Activity::Ptr pActivity );
private:
	Master& m_master;
	std::map< std::shared_ptr< TestClientActivity >, std::uint32_t > m_testsMap;
};


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
class LoadProgram : public megastructure::ExclusiveActivity< LoadProgram >
{
public:
	LoadProgram( Master& master, const std::string& strProgramName ) 
		:	m_master( master ),
			m_strProgramName( strProgramName )
	{
	}
	
	virtual void start();
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message );
private:
	Master& m_master;
	std::string m_strProgramName;
	std::set< std::uint32_t > m_clientIDs;
	bool m_clientFailed = false;
};

}

#endif //ACTIVITIES_26_APRIL_2020