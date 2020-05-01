

#ifndef EG_PROGRAM_01_MAY_2020
#define EG_PROGRAM_01_MAY_2020

#include "megastructure/component.hpp"

#include "egcomponent/egcomponent.hpp"

#include "boost/shared_ptr.hpp"

#include <memory>
#include <string>

namespace megastructure
{
	class Program : public std::enable_shared_from_this< Program >
	{
	public:
		using Ptr = std::shared_ptr< Program >;
	
		Program( Component& component, const std::string& strHostName, const std::string& strProjectName );
		~Program();
		
		void run();
		
	private:
		Component& m_component;
		std::string m_strHostName;
		std::string m_strProjectName;
		
		std::string m_strComponentName;
		boost::filesystem::path m_componentPath;
		
		boost::shared_ptr< megastructure::EGComponent > m_pPlugin;
	};
}

#endif //EG_PROGRAM_01_MAY_2020