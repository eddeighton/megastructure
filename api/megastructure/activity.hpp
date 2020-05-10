
#ifndef ACTIVITY_29_APRIL_2020
#define ACTIVITY_29_APRIL_2020

#include "protocol/megastructure.pb.h"

#include <memory>

namespace megastructure
{

	class Job : public std::enable_shared_from_this< Job >
	{
	public:
		using Ptr = std::shared_ptr< Job >;
		using PtrVector = std::vector< Ptr >;
		
		virtual ~Job();
		virtual void run() = 0;
		virtual bool successful() const = 0;
	};
	
	class Activity : public std::enable_shared_from_this< Activity >
	{
	public:
		using Ptr = std::shared_ptr< Activity >;
		using PtrVector = std::vector< Ptr >;
		using PtrList = std::list< Ptr >;
		
		virtual ~Activity();
		
		virtual void keepAlive();
		virtual bool precondition( PtrList& active );
		virtual void start();
		virtual bool serverMessage( const Message& message );
		virtual bool clientMessage( std::uint32_t uiClient, const Message& message );
		virtual bool jobComplete( Job::Ptr pJob );
		virtual bool activityComplete( Activity::Ptr pActivity );
		virtual void simulationLockGranted();
	};
	
	template< typename ParentType >
	class ExclusiveActivity : public Activity
	{
	public:
		virtual bool precondition( megastructure::Activity::PtrList& active )
		{
			for( auto pActivity : active )
			{
				if( dynamic_cast< const ParentType* >( pActivity.get() ) )
					return false;
			}
			return true;
		}
	};


}

#endif //ACTIVITY_29_APRIL_2020