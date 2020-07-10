
#ifndef COMPONENT_JOBS_01_MAY_2020
#define COMPONENT_JOBS_01_MAY_2020

#include "megastructure/program.hpp"
#include "megastructure/component.hpp"

namespace megastructure
{

	class LoadProgramJob : public Job
	{
	public:
		LoadProgramJob( Component& component, 
            const std::string& strHostName,
            const std::string& strProgramName );
			
		void run();
		bool successful() const { return m_bSuccess; }
	private:
		Component& m_component;
        std::string m_strHostName;
        std::string m_strProgramName;
        
		Program::Ptr m_pNewProgram;
        bool m_bSuccess;
	};


}

#endif //COMPONENT_JOBS_01_MAY_2020