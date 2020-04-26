
#ifndef COMPONENT_16_APRIL_2020
#define COMPONENT_16_APRIL_2020

#pragma once

#include "megastructure/mega.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"

#include "protocol/megastructure.pb.h"

namespace megastructure
{
	std::string getHostProgramName();
	
	class Component
	{
	public:
		Component( const std::string& strSlavePort, const std::string& strProgramName );
		virtual ~Component();
		
		const std::string& getHostProgramName() const { return m_strHostProgram; }
		
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
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		std::thread m_zeromq;
	};
	
}

#endif //COMPONENT_16_APRIL_2020