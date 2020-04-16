
#pragma once

#include "mega.hpp"

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "protocol/megastructure.pb.h"

namespace megastructure
{
	class Component
	{
	public:
		Component( const std::string& strSlaveIP, const std::string& strSlavePort );
		virtual ~Component();
		
		
		
	private:
	
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		std::thread m_zeromq;
	};


}