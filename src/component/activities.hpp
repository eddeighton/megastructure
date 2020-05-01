

#ifndef COMPONENT_ACTIVITIES_26_APRIL_2020
#define COMPONENT_ACTIVITIES_26_APRIL_2020

#include "megastructure/component.hpp"

namespace megastructure
{
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class EnrollHostActivity : public Activity
	{
	public:
		EnrollHostActivity( 
					Component& component,
					const std::string& hostprogram ) 
			:	m_component( component ),
				m_hostprogram( hostprogram )
		{
		}
		
		virtual void start();
		virtual bool serverMessage( const Message& message );
		
	private:
		Component& m_component;
		std::string m_hostprogram;
	};
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class AliveTestActivity : public Activity
	{
	public:
		AliveTestActivity( Component& component ) 
			:	m_component( component )
		{
			
		}
		
		virtual bool serverMessage( const Message& message )
		{
			if( message.has_chq_alive() )
			{
				Message response;
				{
					Message::HCS_Alive* pAlive = response.mutable_hcs_alive();
					pAlive->set_success( true );
				}
				m_component.send( response );
				return true;
			}
			return false;
		}
		
	private:
		Component& m_component;
		std::string m_name;
	};

	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class LoadProgramActivity : public Activity
	{
	public:
		LoadProgramActivity( Component& component ) 
			:	m_component( component )
		{
		}
		
		virtual bool serverMessage( const Message& message );
		virtual bool jobComplete( Job::Ptr pJob );
		
	private:
		Component& m_component;
		Job::Ptr m_pLoadProgramJob;
	};
	
	
}

#endif //COMPONENT_ACTIVITIES_26_APRIL_2020