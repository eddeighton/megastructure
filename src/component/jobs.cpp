
#include "jobs.hpp"

namespace megastructure
{

		
LoadProgramJob::LoadProgramJob( Component& component, 
			Program::Ptr pNewProgram )
		:	m_component( component ),
			m_pNewProgram( pNewProgram )
{

}

void LoadProgramJob::run()
{
	m_component.setProgram( m_pNewProgram );
	
	m_component.jobComplete( shared_from_this() );
}


}