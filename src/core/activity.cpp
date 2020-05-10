
#include "megastructure/activity.hpp"


namespace megastructure
{

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
	Job::~Job()
	{
	}
	
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
	Activity::~Activity()
	{
	}
	
	void Activity::keepAlive()
	{
	}
	
	bool Activity::precondition( PtrList& active )
	{
		return true;
	}
	
	void Activity::start()
	{
	}
	
	bool Activity::serverMessage( const Message& message )
	{
		return false;
	}
	
	bool Activity::clientMessage( std::uint32_t uiClient, const Message& message )
	{
		return false;
	}
	
	bool Activity::jobComplete( Job::Ptr pJob )
	{
		return false;
	}
	
	bool Activity::activityComplete( Activity::Ptr pActivity )
	{
		return false;
	}

	void Activity::simulationLockGranted()
	{
	}
}