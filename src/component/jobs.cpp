
#include "jobs.hpp"

namespace megastructure
{

		
LoadProgramJob::LoadProgramJob( Component& component, 
            const std::string& strHostName,
            const std::string& strProgramName  )
		:	m_component( component ),
			m_strHostName( strHostName ),
			m_strProgramName( strProgramName ),
            m_bSuccess( false )
{

}

void LoadProgramJob::run()
{
    try
    {
        if( !m_strHostName.empty() && !m_strProgramName.empty() )
        {
            m_pNewProgram.reset( 
                new Program( m_component, 
                             m_strHostName,
                             m_strProgramName ) );
        }
        else
        {
            std::cout << "Unloading program" << std::endl;
        }
        m_bSuccess = true;
    }
    catch( std::exception& ex )
    {
        std::cout << "Encountered error while attempting to load program: " << ex.what() << std::endl;
        m_pNewProgram.reset();
    }        

	m_component.setProgram( m_pNewProgram );
	
	m_component.jobComplete( shared_from_this() );
}


}