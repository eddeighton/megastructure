
#pragma once

#include "mega.hpp"

#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "protocol/megastructure.pb.h"

namespace megastructure
{
	std::string getHostProgramName();
	
	class Component;
	
	class EnrollHostActivity : public Activity
	{
	public:
		EnrollHostActivity( 
					Component& component,
					Queue& queue, 
					Client& client,
					const std::string& hostprogram ) 
			:	m_queue( queue ),
				m_component( component ),
				m_client( client ),
				m_hostprogram( hostprogram )
		{
		}
		
		virtual void start()
		{
			std::cout << "EnrollActivity started" << std::endl;
			
			Message message;
			{
				Message::HCQ_Enroll* pEnroll = message.mutable_hcq_enroll();
				pEnroll->set_processname( m_hostprogram );
			}
			m_client.send( message );
		}
		
		virtual bool serverMessage( const Message& message )
		{
			if( message.has_chs_enroll() )
			{
				const Message::CHS_Enroll& enroll = message.chs_enroll();
				std::cout << 
					"Enroll response success : " << enroll.success() << "\n" <<
					"host path: " << enroll.hostpath() << "\n" <<
					"program: " << enroll.program() << std::endl;
				m_queue.activityComplete( shared_from_this() );
				return true;
			}
			return false;
		}
		
	private:
		megastructure::Queue& m_queue;
		Component& m_component;
		Client& m_client;
		std::string m_hostprogram;
	};
	
	class Component
	{
	public:
		Component( const std::string& strSlavePort, const std::string& strProgramName );
		virtual ~Component();
		
		const std::string& getHostProgramName() const { return m_strHostProgram; }
		
		void startActivity( Activity* pActivity )
		{
			m_queue.startActivity( Activity::Ptr( pActivity ) );
		}
		
		bool send( Message& message )
		{
			return m_client.send( message );
		}
		
	private:
		std::string m_strHostProgram;
		megastructure::Queue m_queue;
		megastructure::Client m_client;
		std::thread m_zeromq;
	};


}