
#ifndef COMPONENT_16_APRIL_2020
#define COMPONENT_16_APRIL_2020

#pragma once

#include "megastructure/mega.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"

#include "protocol/megastructure.pb.h"

#include "boost/filesystem/path.hpp"

namespace megastructure
{
	std::string getHostProgramName();
	
	class Component
	{
	public:
		Component( const std::string& strSlavePort, const std::string& strProgramName );
		virtual ~Component();
		
		const std::string& getHostProgramName() const { return m_strHostProgram; }
		const boost::filesystem::path& getSlaveWorkspacePath() const { return m_slaveWorkspacePath; }
		
		void setSlaveWorkspacePath( const boost::filesystem::path& slaveWorkspacePath ) { m_slaveWorkspacePath = slaveWorkspacePath; }
		
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
		
		bool send( megastructure::Message& message )
		{
			return m_client.send( message);
		}
		
	private:
		std::string m_strHostProgram;
		boost::filesystem::path m_slaveWorkspacePath;
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		std::thread m_zeromq;
	};
	
}

#endif //COMPONENT_16_APRIL_2020