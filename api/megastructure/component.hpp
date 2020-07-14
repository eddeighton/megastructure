
#ifndef COMPONENT_16_APRIL_2020
#define COMPONENT_16_APRIL_2020

#include "megastructure/mega.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/log.hpp"

#include "egcomponent/egcomponent.hpp"

#include "schema/project.hpp"

#include "protocol/megastructure.pb.h"

#include "boost/filesystem/path.hpp"

#include <mutex>
#include <list>
#include <mutex>
#include <condition_variable>

namespace megastructure
{
	std::string getHostProgramName();
	
	class Program;
	
	class Component
	{
		friend class EnrollHostActivity;
		friend class AliveTestActivity;
		friend class LoadProgramActivity;
		friend class BufferActivity;
		
		friend class TestEGReadActivity;
		friend class TestEGWriteActivity;
		friend class TestEGCallActivity;
		friend class EGRequestManagerActivity;
		friend class EGRequestHandlerActivity;
        
		friend class LoadProgramJob;
        
        friend class Program;
		
		friend class SimThreadManager;
		
	public:
		Component( Environment& environment, const std::string& strMegaPort, const std::string& strEGPort, const std::string& strProgramName );
		virtual ~Component();
		
		Environment& getEnvironment() { return m_environment; }
		
		const std::string& getHostProgramName() const { return m_strHostProgram; }
		const std::string& getSlaveName() const { return m_strSlaveName; }
		const boost::filesystem::path& getSlaveWorkspacePath() const { return m_slaveWorkspacePath; }
		std::shared_ptr< Program > getProgram() { return m_pProgram; }
		
		void runCycle();
        
	private:
		void setSlaveName( const std::string& strSlaveName ) { m_strSlaveName = strSlaveName; }
		void setSlaveWorkspacePath( const boost::filesystem::path& slaveWorkspacePath ) { m_slaveWorkspacePath = slaveWorkspacePath; }
		
		//activities
		void startActivity( megastructure::Activity::Ptr pActivity )
		{
			m_queue.startActivity( pActivity );
		}
		void startActivity( Activity* pActivity )
		{
			m_queue.startActivity( Activity::Ptr( pActivity ) );
		}
		void activityComplete( Activity::Ptr pActivity )
		{
			m_queue.activityComplete( pActivity );
		}
		
		//simulation lock
		void requestSimulationLock( Activity::Ptr pActivity );
		void releaseSimulationLock( Activity::Ptr pActivity );
		void grantNextSimulationLock();
		
		//jobs
		void startJob( Job::Ptr pJob );
		void jobComplete( Job::Ptr pJob );
		void runAllJobs();
		
		//program
		void setProgram( std::shared_ptr< Program > pProgram );
        std::future< std::string > getSharedBuffer( const std::string& strName, std::size_t szSize );
		
		//megastructure protocol
		void send( const megastructure::Message& message )
		{
			if( !m_client.send( message) )
			{
				//disconnect...
				throw std::runtime_error( "Socket failed" );
			}
		}
		void sendeg( const megastructure::Message& message )
		{
			if( !m_egClient.send( message ) )
			{
				//disconnect...
				throw std::runtime_error( "Socket failed" );
			}
		}
		bool readeg( megastructure::Message& message )
		{
			bool bReceived = false;
			if( m_egClient.recv_async( message, bReceived ) )
			{
				return bReceived;
			}
			else
			{
				throw std::runtime_error( "Socket failed" );
			}
		}
		bool readegSync( megastructure::Message& message )
		{
			bool bReceived = false;
			if( m_egClient.recv_sync( message, bReceived ) )
			{
				return bReceived;
			}
			else
			{
				throw std::runtime_error( "Socket failed" );
			}
		}
		
	private:
		Environment m_environment;
		std::string m_strHostProgram;
		std::string m_strSlaveName;
		boost::filesystem::path m_slaveWorkspacePath;
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		megastructure::Client m_egClient;
		std::thread m_zeromq1;
		
		//simulation lock
		Activity::PtrList m_simulationLockActivities;
		std::mutex m_simulationMutex;
		Activity::Ptr m_pLockActivity;
		bool m_bSimulationHasLock;
		std::list< Job::Ptr > m_jobs;
		std::condition_variable m_simJobCondition;
		
		std::shared_ptr< Program > m_pProgram;
        
        std::shared_ptr< spdlog::details::thread_pool > m_logThreadPool;
	};
	
}

#endif //COMPONENT_16_APRIL_2020