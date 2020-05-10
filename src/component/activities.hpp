

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
		Job::Ptr m_pLoadProgramJob;
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
	
	class EGRequestHandlerActivity : public Activity
	{
	public:
		using Ptr = std::shared_ptr< EGRequestHandlerActivity >;
		
		EGRequestHandlerActivity( Component& component );
		void request( const Message& message );
		void simulationLockGranted();
	private:
		void processMessages();
	
		Component& m_component;
		std::vector< Message > m_messages;
		bool m_bHasSimulationLock;
		
	};
	
	class EGRequestManagerActivity : public Activity
	{
		using CoordinatorHostCycle = 
			std::tuple< std::uint32_t, std::uint32_t, std::uint32_t >;
		
		using ActivityMap =
			std::map< CoordinatorHostCycle, EGRequestHandlerActivity::Ptr >;
		
	public:
		EGRequestManagerActivity( Component& component ) 
			:	m_component( component )
		{
		}
		
		virtual bool serverMessage( const Message& message );
		virtual bool activityComplete( Activity::Ptr pActivity );
		
	private:
		Component& m_component;
		ActivityMap m_activities;
		
	};
}

#endif //COMPONENT_ACTIVITIES_26_APRIL_2020