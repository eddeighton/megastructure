

#include "activities.hpp"
#include "program.hpp"

#include "megastructure/component.hpp"
#include "megastructure/clientServer.hpp"

#include "common/assert_verify.hpp"

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>

#include <functional>


namespace megastructure
{
	std::string getHostProgramName()
	{
		boost::filesystem::path currentProgramPath = boost::dll::program_location();
		boost::filesystem::path filename = currentProgramPath.filename();
		return filename.string();
	}
	
	Component::Component( Environment& environment, const std::string& strMegaPort, const std::string& strEGPort, const std::string& strProgramName )
		:	m_environment( environment ),
			m_strHostProgram( strProgramName ),
			m_queue(),
			m_client( TCPRemoteSocketName( "localhost", strMegaPort ) ),
			m_egClient( TCPRemoteSocketName( "localhost", strEGPort ) )
	{
		m_zeromq1 = std::thread( 
			std::bind( &megastructure::readClient< megastructure::Client >, 
				std::ref( m_client ), std::ref( m_queue ) ) );
				
		m_queue.startActivity( new EnrollHostActivity( *this, m_strHostProgram ) );
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new LoadProgramActivity( *this ) );
		m_queue.startActivity( new EGRequestManagerActivity( *this ) );
	}
	
	Component::~Component()
	{
		m_queue.stop();
		m_client.stop();
		m_egClient.stop();
		m_zeromq1.join();
	}
	
	class SimThreadManager
	{
		Component& m_component;
		bool m_bLocked;
	public:
		SimThreadManager( Component& component )
			:	m_component( component ),
				m_bLocked( false )
		{
			//either immediately acquire the simulation lock or fail to and return false
			if( m_component.m_simThreadMutex.try_lock() )
			{
				m_bLocked = true;
				runAllJobs();
			}
			else
			{
				//do not return until we acquire the simulation lock
				bool& bLocked = m_bLocked;
				while( !bLocked )
				{
					runAllJobs();
					//wait for more jobs OR completion
					{
						std::unique_lock< std::mutex > simLock( m_component.m_jobMutex );
						m_component.m_simJobCondition.wait
						( 
							simLock, 
							[ &component, &bLocked ]
							{ 
								if( !component.m_jobs.empty() )
								{
									return true;
								}
								else if( component.m_simThreadMutex.try_lock() )
								{
									bLocked = true;
									return true;
								}
								else
								{
									return false;
								}
							} 
						);
					}
				}
			}
		}
		
		~SimThreadManager()
		{
			//release simulation lock if acquired
			if( m_bLocked )
			{
				m_component.m_simThreadMutex.unlock();
			}
		}
		
		void runAllJobs()
		{
			std::list< Job::Ptr > temp;
			{
				std::unique_lock< std::mutex > simLock( m_component.m_jobMutex );
				temp.swap( m_component.m_jobs );
			}
			if( !temp.empty() )
			{
				//process all jobs
				for( Job::Ptr pJob : temp )
					pJob->run();
				temp.clear();
			}
		}
	
		void spinProtection()
		{
			//in the case that simulation lock WAS requested while it WAS acquired by this then 
			//ensure the requester GETS IT before continuing by waiting on condition variable
		}
	};
	
	void Component::runCycle()
	{
		SimThreadManager simThreadManager( *this );
		
		//run simulation
		if( m_pProgram )
		{
			m_pProgram->run();
		}
		
		simThreadManager.spinProtection();
		
	}
	
	std::string Component::egRead( std::uint32_t uiType, std::uint32_t uiInstance )
	{
		if( m_pProgram )
		{
			return m_pProgram->egRead( uiType, uiInstance );
		}
		else
		{
			THROW_RTE( "EG Read called when no active program" );
		}
	}
	
	void Component::egWrite( std::uint32_t uiType, std::uint32_t uiInstance, const std::string& strBuffer )
	{
		if( m_pProgram )
		{
			return m_pProgram->egWrite( uiType, uiInstance, strBuffer );
		}
		else
		{
			THROW_RTE( "EG Write called when no active program" );
		}
	}
	
	void Component::egCall( std::uint32_t uiType, std::uint32_t uiInstance, const std::string& strBuffer )
	{
		if( m_pProgram )
		{
			return m_pProgram->egCall( uiType, uiInstance, strBuffer );
		}
		else
		{
			THROW_RTE( "EG Call called when no active program" );
		}
	}
		
	void Component::startJob( Job::Ptr pJob )
	{
		std::unique_lock< std::mutex > simLock( m_jobMutex );
		m_jobs.push_back( pJob );
		m_simJobCondition.notify_one();
	}
	
	void Component::jobComplete( Job::Ptr pJob )
	{
		m_queue.jobComplete( pJob );
	}
	
	//only called in main thread
	void Component::setProgram( Program::Ptr pProgram )
	{
		m_pProgram = pProgram;
	}
    
    std::future< std::string > Component::getSharedBuffer( const std::string& strName, std::size_t szSize )
    {
        //std::cout << "Creating BufferActivity for buffer: " << strName << " size: " << szSize << std::endl;
        BufferActivity* pBufferActivity = 
            new BufferActivity( *this, strName, szSize );
            
        startActivity( pBufferActivity );
        
        return pBufferActivity->getSharedBufferName();
    }
	
	
}
