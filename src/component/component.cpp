

#include "megastructure/component.hpp"

#include <functional>

namespace megastructure
{
	Component::Component( const std::string& strSlaveIP, const std::string& strSlavePort )
		:	m_queue(),
			m_client( strSlaveIP, strSlavePort )
	{
		m_zeromq = std::thread( 
			std::bind( &megastructure::readClient< megastructure::Client >, std::ref( m_client ), std::ref( m_queue ) ) );
	}
	
	Component::~Component()
	{
		m_queue.stop();
	}
}
