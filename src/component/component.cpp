

#include "megastructure/component.hpp"
#include "megastructure/clientServer.hpp"

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>

#include <functional>

#include "activities.hpp"
#include "program.hpp"

namespace megastructure
{
	std::string getHostProgramName()
	{
		boost::filesystem::path currentProgramPath = boost::dll::program_location();
		boost::filesystem::path filename = currentProgramPath.filename();
		return filename.string();
	}
	
	Component::Component( const std::string& strSlavePort, const std::string& strProgramName )
		:	m_strHostProgram( strProgramName ),
			m_queue(),
			m_client( "localhost", strSlavePort )
	{
		m_zeromq = std::thread( 
			std::bind( &megastructure::readClient< megastructure::Client >, 
				std::ref( m_client ), std::ref( m_queue ) ) );
				
		m_queue.startActivity( new EnrollHostActivity( *this, m_strHostProgram ) );
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new LoadProgramActivity( *this ) );
	}
	
	Component::~Component()
	{
		m_queue.stop();
		m_client.stop();
		m_zeromq.join();
	}
	
	
	void Component::runCycle()
	{
		Job::Ptr pJob;
		{
			std::lock_guard< std::mutex > lock( m_simThreadMutex );
			if( !m_jobs.empty() )
			{
				pJob = m_jobs.front();
			}
		}
		
		if( pJob )
		{
			pJob->run();
		}
		
		if( m_pProgram )
		{
			m_pProgram->run();
		}
		
	}
		
	void Component::startJob( Job::Ptr pJob )
	{
		std::lock_guard< std::mutex > lock( m_simThreadMutex );
		m_jobs.push_back( pJob );
	}
	
	void Component::jobComplete( Job::Ptr pJob )
	{
		{
			std::lock_guard< std::mutex > lock( m_simThreadMutex );
			for( std::list< Job::Ptr >::iterator i = m_jobs.begin(),
				iEnd = m_jobs.end(); i!=iEnd; ++i )
			{
				if( *i == pJob )
				{
					m_jobs.erase( i );
					break;
				}
			}
		}
		m_queue.jobComplete( pJob );
	}
	
	//only called in main thread
	void Component::setProgram( Program::Ptr pProgram )
	{
		m_pProgram = pProgram;
	}
}
