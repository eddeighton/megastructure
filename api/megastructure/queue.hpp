

#ifndef MEGASTRUCTURE_QUEUE_15_04_2020
#define MEGASTRUCTURE_QUEUE_15_04_2020

#include "protocol/megastructure.pb.h"

#include "boost/asio.hpp"

#include <memory>
#include <vector>
#include <list>
#include <thread>

namespace megastructure
{



	class Job
	{
	public:
		using Ptr = std::shared_ptr< Job >;
	
		virtual ~Job();
		
	};
	
	class Activity : public std::enable_shared_from_this< Activity >
	{
	public:
		using Ptr = std::shared_ptr< Activity >;
		using PtrVector = std::vector< Ptr >;
		using PtrList = std::list< Ptr >;
		
		virtual ~Activity();
		
		virtual void keepAlive()
		{
		}
		virtual bool precondition( PtrList& active )
		{
			return true;
		}
		
		virtual void start()
		{
		}
		
		virtual bool serverMessage( const Message& message )
		{
			return false;
		}
		virtual bool clientMessage( std::uint32_t uiClient, const Message& message )
		{
			return false;
		}
		virtual bool jobComplete( Job::Ptr pJob )
		{
			return false;
		}
		virtual bool activityComplete( Activity::Ptr pActivity )
		{
			return false;
		}
	};
	
	
	class Queue
	{
	public:
		Queue();
		~Queue();
		
		void stop()
		{
			m_stop = true;
		}
		
		void startActivity( Activity::Ptr pActivity );
		void startActivity( Activity* pActivity )
		{
			startActivity( Activity::Ptr( pActivity ) );
		}
		
		void serverMessage( const Message& message );
		void clientMessage( std::uint32_t uiClient, const Message& message );
		void jobComplete( Job::Ptr pJob );
		void activityComplete( Activity::Ptr pActivity );
		
	private:
		void runQueue();
		void OnKeepAlive( const boost::system::error_code& ec );
		void OnStartActivity( Activity::Ptr pActivity );
		void OnServerMessage( Message message );
		void OnClientMessage( std::uint32_t uiClient, Message message );
		void OnJobComplete( Job::Ptr pJob );
		void OnActivityComplete( Activity::Ptr pActivity );
		
	protected:
		std::atomic< bool > m_stop;
		boost::asio::io_context m_queue;
		boost::asio::steady_timer m_keepAliveTimer;
		std::thread m_queueThread;
		Activity::PtrList m_active, m_pending;
	};
	

	template< class Socket >
	void readServer( Socket& readSocket, Queue& queue )
	{
		try
		{
			while( true )
			{
				std::cout << ".";
				
				std::string str;
				std::uint32_t uiClient = 0;
				megastructure::Message message;
				bool bReceived = false;
				if( readSocket.recv_sync( message, uiClient, bReceived ) )
				{
					if( bReceived )
						queue.clientMessage( uiClient, message );
				}
				else
				{
					break;
				}
			}
		}
		catch( std::exception& ex )
		{
			std::cout << "Exception in server function: " << ex.what() << std::endl;
		}
		catch( ... )
		{
			std::cout << "Unknown exception in server function" << std::endl;
		}
	}
	

	template< class Socket >
	void readClient( Socket& readSocket, Queue& queue )
	{
		try
		{
			while( true )
			{
				std::cout << ".";
				
				megastructure::Message message;
				bool bReceived = false;
				if( readSocket.recv_sync( message, bReceived ) )
				{
					if( bReceived )
						queue.serverMessage( message );
				}
				else
				{
					break;
				}
			}
		}
		catch( std::exception& ex )
		{
			std::cout << "Exception in server function: " << ex.what() << std::endl;
		}
		catch( ... )
		{
			std::cout << "Unknown exception in server function" << std::endl;
		}
	}


}

#endif //MEGASTRUCTURE_QUEUE_15_04_2020