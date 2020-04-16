

#include "megastructure/component.hpp"
#include "megastructure/clientServer.hpp"

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
	
	Component::Component( const std::string& strSlavePort, const std::string& strProgramName )
		:	m_strHostProgram( strProgramName ),
			m_queue(),
			m_client( "localhost", strSlavePort )
	{
		m_zeromq = std::thread( 
			std::bind( &megastructure::readClient< megastructure::Client >, 
				std::ref( m_client ), std::ref( m_queue ) ) );
				
		m_queue.startActivity( new EnrollHostActivity( *this, m_queue, m_client, m_strHostProgram ) );
	}
	
	Component::~Component()
	{
		m_queue.stop();
	}
}
