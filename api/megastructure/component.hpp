
#ifndef COMPONENT_16_APRIL_2020
#define COMPONENT_16_APRIL_2020

#pragma once

#include "megastructure/mega.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"

#include "egcomponent/egcomponent.hpp"

#include "protocol/megastructure.pb.h"

#include "boost/filesystem/path.hpp"

#include <mutex>
#include <list>

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
        
		friend class LoadProgramJob;
        
        friend class Program;
	public:
		Component( const std::string& strMegaPort, const std::string& strEGPort, const std::string& strProgramName );
		virtual ~Component();
		
		const std::string& getHostProgramName() const { return m_strHostProgram; }
		const std::string& getSlaveName() const { return m_strSlaveName; }
		const boost::filesystem::path& getSlaveWorkspacePath() const { return m_slaveWorkspacePath; }
		
		void runCycle();
		
		//test eg protocol routines
		std::future< std::string > egRead( std::uint32_t uiType, std::uint32_t uiInstance );
		void egWrite( std::uint32_t uiType, std::uint32_t uiInstance, const std::string& strBuffer );
		void egCall( std::uint32_t uiType, std::uint32_t uiInstance, const std::string& strBuffer );
		
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
		
		//jobs
		void startJob( Job::Ptr pJob );
		void jobComplete( Job::Ptr pJob );
		
		//program
		void setProgram( std::shared_ptr< Program > pProgram );
        std::future< std::string > getSharedBuffer( const std::string& strName, std::size_t szSize );
		
		//megastructure protocol
		bool send( megastructure::Message& message )
		{
			return m_client.send( message);
		}
		bool sendeg( megastructure::Message& message )
		{
			return m_egClient.send( message );
		}
		
	private:
		std::string m_strHostProgram;
		std::string m_strSlaveName;
		boost::filesystem::path m_slaveWorkspacePath;
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		megastructure::Client m_egClient;
		std::thread m_zeromq1;
		std::thread m_zeromq2;
		
		std::mutex m_simThreadMutex;
		std::list< Job::Ptr > m_jobs;
		
		std::shared_ptr< Program > m_pProgram;
	};
	
}

#endif //COMPONENT_16_APRIL_2020