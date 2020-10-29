
#include "jobs.hpp"

#include "common/assert_verify.hpp"

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
            if( m_pNewProgram )
            {
                m_pNewProgram.reset();
                SPDLOG_INFO( "Unloaded program" );
            }
        }
        m_bSuccess = true;
    }
    catch( std::exception& ex )
    {
        SPDLOG_ERROR( "Encountered error while attempting to load program: {}", ex.what() );
        m_pNewProgram.reset();
    }        

	m_component.setProgram( m_pNewProgram );
	
	m_component.jobComplete( shared_from_this() );
}


ConfigJob::ConfigJob( Component& component, ConfigActivityType type )
		:	m_component( component ),
			m_type( type ),
            m_bSuccess( false )
{
}

void ConfigJob::run()
{
    try
    {
        switch( m_type )
        {
            case eConfigLoading:
                m_component.loadConfig();
                break;
            case eConfigSaving:
                m_component.saveConfig();
                break;
            case TOTAL_CONFIG_ACTIVITY_TYPES:
                THROW_RTE( "Unsupported config activity type" );
        }
        m_bSuccess = true;
    }
    catch( std::exception& ex )
    {
        SPDLOG_ERROR( "Encountered error while attempting perform config job: {}", ex.what() );
    }        

	m_component.jobComplete( shared_from_this() );
}


}