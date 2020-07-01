

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
	
	
	void Component::grantNextSimulationLock()
	{
		//assumes m_simulationMutex is locked
		if( !m_bSimulationHasLock )
		{
			if( !m_pLockActivity )
			{
				if( !m_simulationLockActivities.empty() )
				{
					m_pLockActivity = m_simulationLockActivities.front();
					m_simulationLockActivities.pop_front();
					m_queue.simulationLockGranted( m_pLockActivity );
				}
			}
		}
	}
	
	void Component::requestSimulationLock( Activity::Ptr pActivity )
	{
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
		
		m_simulationLockActivities.push_back( pActivity );
		
		grantNextSimulationLock();
	}
	
	void Component::releaseSimulationLock( Activity::Ptr pActivity )
	{
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
		
		if( pActivity != m_pLockActivity )
		{
			Activity::PtrList::iterator iFind =
				std::find( m_simulationLockActivities.begin(), 
					m_simulationLockActivities.end(), pActivity );
			VERIFY_RTE( iFind != m_simulationLockActivities.end() );
			m_simulationLockActivities.erase( iFind );
		}
		else
		{
			m_pLockActivity.reset();
			if( !m_simulationLockActivities.empty() )
				grantNextSimulationLock();
			else
				m_simJobCondition.notify_one();
		}
	}

	void Component::runAllJobs()
	{
		std::list< Job::Ptr > temp;
		{
			std::unique_lock< std::mutex > simLock( m_simulationMutex );
			temp.swap( m_jobs );
		}
		if( !temp.empty() )
		{
			//process all jobs
			for( Job::Ptr pJob : temp )
				pJob->run();
			temp.clear();
		}
	}
	
	void Component::runCycle()
	{
		bool bIsActivityLock = false;
		{
			std::unique_lock< std::mutex > simLock( m_simulationMutex );
			if( m_pLockActivity || !m_jobs.empty() )
			{
				bIsActivityLock = true;
			}
		}
		
		if( bIsActivityLock )
		{
			Component& component = *this;
			
			//enter locked state
			while( true )
			{
				runAllJobs();
				
				//wait for more jobs OR completion
				std::unique_lock< std::mutex > simLock( m_simulationMutex );
				m_simJobCondition.wait
				( 
					simLock, 
					[ &component ]
					{ 
						return !component.m_pLockActivity || !component.m_jobs.empty();
					} 
				);
				if( !m_pLockActivity )
				{
					m_bSimulationHasLock = true;
					break;
				}
			}
		}
	
		//run simulation
		if( m_pProgram )
		{
			m_pProgram->run();
		}
		
		{
			std::unique_lock< std::mutex > simLock( m_simulationMutex );
			m_bSimulationHasLock = false;
			grantNextSimulationLock();
		}
	}
	/*
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
	}*/
		
	void Component::startJob( Job::Ptr pJob )
	{
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
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
