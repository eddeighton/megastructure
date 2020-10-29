

#ifndef COMPONENT_ACTIVITIES_26_APRIL_2020
#define COMPONENT_ACTIVITIES_26_APRIL_2020

#include "megastructure/component.hpp"

#include <tuple>
#include <map>
#include <memory>

namespace megastructure
{
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class EnrollHostActivity : public Activity
	{
	public:
		EnrollHostActivity( Component& component, const std::string& hostprogram );
		
		virtual void start();
		virtual bool serverMessage( const Message& message );
		
	private:
		Component& m_component;
		std::string m_hostprogram;
		std::string m_strUnique;
		
		std::string m_strWorkspace, m_strSlaveName;
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

    class LoadProgramJob;
    
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
		std::string m_strProgramName;
		std::string m_strHostName;
        std::shared_ptr< LoadProgramJob > m_pLoadProgramJob;
	};
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class BufferActivity : public Activity
	{
	public:
		BufferActivity( Component& component,
						const std::string& bufferName,
						std::size_t szBufferSize ) 
			:	m_component( component ),
				m_bufferName( bufferName ),
				m_bufferSize( szBufferSize )
		{
		}
		
		virtual void start();
		virtual bool serverMessage( const Message& message );
		
        std::future< std::string > getSharedBufferName() { return m_result.get_future(); }
	private:
		Component& m_component;
		std::string m_bufferName;
        std::size_t m_bufferSize;
        std::promise< std::string > m_result;
	};
	
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class EGRequestManagerActivity : public Activity
	{
		
	public:
		EGRequestManagerActivity( Component& component ) 
			:	m_component( component )
		{
		}
		
		virtual bool serverMessage( const Message& message );
		virtual bool activityComplete( Activity::Ptr pActivity );
        virtual void simulationLockStarted();
	private:
    
        void OnReadRequest( const Message::EG_Msg& egMsg );
        void OnWriteRequest( const Message::EG_Msg& egMsg );
        void OnLockRequest( const Message::EG_Msg& egMsg );
        void OnUnlockRequest( const Message::EG_Msg& egMsg );
    
        void issueLockResponse( const SimulationLock::HostCycleState& hcs ) const;

		Component& m_component;
		
	};
    
    class ConfigJob;
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	class ConfigActivity : public Activity
	{
	public:
		ConfigActivity( Component& component ) 
			:	m_component( component )
		{
		}
		
		virtual bool serverMessage( const Message& message );
		virtual bool jobComplete( Job::Ptr pJob );
		
	private:
		Component& m_component;
        std::shared_ptr< ConfigJob > m_pConfigJob;
	};
}

#endif //COMPONENT_ACTIVITIES_26_APRIL_2020