

#include "activities.hpp"

#include "megastructure/program.hpp"
#include "megastructure/component.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/log.hpp"

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
	
	Component::Component( Environment& environment, 
                            const std::string& strMegaPort, 
                            const std::string& strEGPort, 
                            const std::string& strProgramName,
                            void* pHostInterface )
		:	m_environment( environment ),
			m_strHostProgram( strProgramName ),
			m_pHostInterface( pHostInterface ),
            m_queue(),
			m_client( TCPRemoteSocketName( "localhost", strMegaPort ) ),
			m_egClient( TCPRemoteSocketName( "localhost", strEGPort ) ),
            m_bCurrentLockReleased( false )
	{
        m_logThreadPool = megastructure::configureLog( m_environment.getLogFolderPath(), strProgramName );
        
        SPDLOG_INFO( "Created megastructure component with mega port:{} eg port:{} program name:{}", strMegaPort, strEGPort, strProgramName );
        
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
        SPDLOG_INFO( "Megastructure component shutdown {}", m_strHostProgram );
        
		m_queue.stop();
		m_client.stop();
		m_egClient.stop();
		m_zeromq1.join();
        
        spdlog::drop( m_strHostProgram );
        m_logThreadPool.reset();
	}
	
    
    SimulationLock::Ptr Component::getCurrentLock()
    {
        //called from message queue thread
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
        return m_pCurrentSimLock;
    }
    SimulationLock::Ptr Component::getOrCreateCurrentLock()
    {
        //called from message queue thread
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
        if( !m_pCurrentSimLock )
        {
            if( m_pFutureSimLock )
                m_pFutureSimLock.swap( m_pCurrentSimLock );
            else
                m_pCurrentSimLock = std::make_shared< SimulationLock >();
        }
        return m_pCurrentSimLock;
    }
    
    SimulationLock::Ptr Component::getOrCreateFutureLock()
    {
        //called from message queue thread
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
        if( !m_pFutureSimLock )
        {
            m_pFutureSimLock = std::make_shared< SimulationLock >();
        }
        return m_pFutureSimLock;
    }
    
    void Component::releaseCurrentLock()
    {
        //called from message queue thread
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
        
        m_pCurrentSimLock.reset();
        if( !m_pCurrentSimLock && m_pFutureSimLock )
        {
            m_pFutureSimLock.swap( m_pCurrentSimLock );
        }
        
        m_bCurrentLockReleased = true;
        m_simJobCondition.notify_one();
    }    

	void Component::runAllJobs()
	{
		std::list< Job::Ptr > temp;
		{
			std::unique_lock< std::mutex > simLock( m_simulationMutex );
			temp.swap( m_jobs );
		}
        for( Job::Ptr pJob : temp )
            pJob->run();
	}
    
    void* Component::getRoot()
    {
        return m_pProgram ? m_pProgram->getRoot() : nullptr;
    }
	
	void Component::runCycle()
	{
        runAllJobs();
		
        {
            std::unique_lock< std::mutex > simLock( m_simulationMutex );
            
            if( m_pCurrentSimLock )
            {
                m_queue.startSimulationLock();
                m_bCurrentLockReleased = false;
                const bool& bCurrentLockReleased = m_bCurrentLockReleased;
                m_simJobCondition.wait
                ( 
                    simLock, 
                    [ &bCurrentLockReleased ]
                    { 
                        return bCurrentLockReleased;
                    } 
                );
            }
        }
	
		//run simulation
		if( m_pProgram )
		{
			m_pProgram->run();
		}
	}
    
	void Component::startJob( Job::Ptr pJob )
	{
		std::unique_lock< std::mutex > simLock( m_simulationMutex );
		m_jobs.push_back( pJob );
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
        SPDLOG_TRACE( "Creating BufferActivity for buffer: {} size: {}", strName, szSize );
        BufferActivity* pBufferActivity = 
            new BufferActivity( *this, strName, szSize );
            
        startActivity( pBufferActivity );
        
        return pBufferActivity->getSharedBufferName();
    }
	
	
}
