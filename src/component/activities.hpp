

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
	/*
	class TestEGReadActivity : public Activity
	{
	public:
		TestEGReadActivity( Component& component, 
						std::uint32_t uiType, 
						std::uint32_t uiInstance) 
			:	m_component( component ),
				m_uiType( uiType ),
				m_uiInstance( uiInstance )
		{
		}
		
		std::future< std::string > getResult() { return m_resultPromise.get_future(); }
		
		virtual void start();
		virtual bool serverMessage( const Message& message );
	private:
		Component& m_component;
		std::uint32_t 	m_uiType;
		std::uint32_t 	m_uiInstance;
		std::promise< std::string > m_resultPromise;
	};
	
	class TestEGWriteActivity : public Activity
	{
	public:
		TestEGWriteActivity( Component& component, 
						std::uint32_t uiType, 
						std::uint32_t uiInstance, 
						const std::string& strBuffer ) 
			:	m_component( component ),
				m_uiType( uiType ),
				m_uiInstance( uiInstance ),
				m_strBuffer( strBuffer )
		{
		}
		
		virtual void start();
	private:
		Component& m_component;
		std::uint32_t 	m_uiType;
		std::uint32_t 	m_uiInstance;
		std::string 	m_strBuffer;
	};
	
	class TestEGCallActivity : public Activity
	{
	public:
		TestEGCallActivity( Component& component, 
						std::uint32_t uiType, 
						std::uint32_t uiInstance, 
						const std::string& strBuffer ) 
			:	m_component( component ),
				m_uiType( uiType ),
				m_uiInstance( uiInstance ),
				m_strBuffer( strBuffer )
		{
		}
		
		virtual void start();
	private:
		Component& m_component;
		std::uint32_t 	m_uiType;
		std::uint32_t 	m_uiInstance;
		std::string 	m_strBuffer;
	};*/
}

#endif //COMPONENT_ACTIVITIES_26_APRIL_2020