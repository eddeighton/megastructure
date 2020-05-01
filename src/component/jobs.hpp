
#ifndef COMPONENT_JOBS_01_MAY_2020
#define COMPONENT_JOBS_01_MAY_2020

#include "program.hpp"

#include "megastructure/component.hpp"

namespace megastructure
{

	class LoadProgramJob : public Job
	{
	public:
		LoadProgramJob( Component& component, 
			Program::Ptr pNewProgram );
			
		void run();
		
	private:
		Component& m_component;
		Program::Ptr m_pNewProgram;
	};


}

#endif //COMPONENT_JOBS_01_MAY_2020