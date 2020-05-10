

#include "megastructure/queue.hpp"

#include "common/assert_verify.hpp"
#include "common/requireSemicolon.hpp"

#include <boost/system/error_code.hpp>
#include "boost/current_function.hpp"

#include <iostream>

using namespace std::chrono_literals;
namespace
{
	static const auto KEEP_ALIVE_RATE = 250ms;
}
	
namespace megastructure
{
	
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

#define PRINTEXCEPTION_AND_ABORT( code )                                                  \
	DO_STUFF_AND_REQUIRE_SEMI_COLON(                                                      \
		try                                                                               \
		{                                                                                 \
			code                                                                          \
		}                                                                                 \
		catch( std::exception& ex )                                                       \
		{                                                                                 \
			std::cout << BOOST_CURRENT_FUNCTION " exception: " << ex.what() << std::endl; \
			std::abort();                                                                 \
		}                                                                                 \
		catch( ... )                                                                      \
		{                                                                                 \
			std::cout << BOOST_CURRENT_FUNCTION " Unknown exception" << std::endl;        \
		}                                                                                 \
	)

	Queue::Queue()
		:	m_stop( false ),
			m_keepAliveTimer( m_queue, KEEP_ALIVE_RATE )
	{
		{
			using namespace std::placeholders;
			m_keepAliveTimer.async_wait( std::bind( &Queue::OnKeepAlive, this, _1 ) );
		}
		
		boost::asio::io_context* queue = &m_queue;
		m_queueThread = std::thread( [ queue ](){ queue->run();} );
	}

	Queue::~Queue()
	{
		stop();
		m_queueThread.join();
	}

	void Queue::OnKeepAlive( const boost::system::error_code& ec )
	{
		PRINTEXCEPTION_AND_ABORT
		(
			for( Activity::Ptr pActivity : m_active )
			{
				pActivity->keepAlive();
			}
		);
		
		if( !m_stop && ec.value() == boost::system::errc::success )
		{
			m_keepAliveTimer.expires_at( m_keepAliveTimer.expiry() + KEEP_ALIVE_RATE );
			using namespace std::placeholders;
			m_keepAliveTimer.async_wait( std::bind( &Queue::OnKeepAlive, this, _1 ) );
		}
	}
	
	
	void Queue::startActivity( Activity::Ptr pActivity )
	{
		m_queue.post( std::bind( &Queue::OnStartActivity, this, pActivity ) );
	}
	
	void Queue::OnStartActivity( Activity::Ptr pActivity )
	{
		if( m_pending.empty() && pActivity->precondition( m_active ) )
		{
			m_active.push_back( pActivity );
			PRINTEXCEPTION_AND_ABORT( pActivity->start(); );
		}
		else
		{
			m_pending.push_back( pActivity );
		}
	}
	
	void Queue::serverMessage( const Message& message )
	{
		m_queue.post( std::bind( &Queue::OnServerMessage, this, message ) );
	}
	void Queue::OnServerMessage( Message message )
	{
		for( Activity::Ptr pActivity : m_active )
		{
			PRINTEXCEPTION_AND_ABORT
			(
				if( pActivity->serverMessage( message ) )
					break;
			);
		}
	}
	
	void Queue::clientMessage( std::uint32_t uiClient, const Message& message )
	{
		m_queue.post( std::bind( &Queue::OnClientMessage, this, uiClient, message ) );
	}
	void Queue::OnClientMessage( std::uint32_t uiClient, Message message )
	{
		for( Activity::Ptr pActivity : m_active )
		{
			PRINTEXCEPTION_AND_ABORT
			(
				if( pActivity->clientMessage( uiClient, message ) )
					break;
			);
		}
	}
	
	
	void Queue::jobComplete( Job::Ptr pJob )
	{
		m_queue.post( std::bind( &Queue::OnJobComplete, this, pJob ) );
	}
	void Queue::OnJobComplete( Job::Ptr pJob )
	{
		for( Activity::Ptr pActivity : m_active )
		{
			PRINTEXCEPTION_AND_ABORT
			(
				if( pActivity->jobComplete( pJob ) )
					break;
			);
		}
	}
	
	void Queue::activityComplete( Activity::Ptr pActivity )
	{
		m_queue.post( std::bind( &Queue::OnActivityComplete, this, pActivity ) );
	}
	void Queue::OnActivityComplete( Activity::Ptr pActivity )
	{
		bool bFound = false;
		for( Activity::PtrList::iterator i = m_active.begin(),
			iEnd = m_active.end(); i!=iEnd;  )
		{
			if( *i == pActivity )
			{
				i = m_active.erase( i );
				bFound = true;
				break;
			}
			else
			{
				PRINTEXCEPTION_AND_ABORT
				(
					if( (*i)->activityComplete( pActivity ) && bFound )
					{
						break;
					}
				);
				++i;
			}
		}
		VERIFY_RTE_MSG( bFound, "Failed to find activity" );
		
		//assess whether pending now viable
		if( !m_pending.empty() )
		{
			Activity::Ptr pFirstPending = m_pending.front();
			if( pFirstPending->precondition( m_active ) )
			{
				m_pending.pop_front();
				m_active.push_back( pFirstPending );
				PRINTEXCEPTION_AND_ABORT( pFirstPending->start(); );
			}
		}
		
		if( m_active.empty() && m_pending.empty() )
		{
			std::cout << "activities complete" << std::endl;
		}
	}
	
	
	void Queue::simulationLockGranted( Activity::Ptr pActivity )
	{
		m_queue.post( std::bind( &Queue::OnSimulationLockGranted, this, pActivity ) );
	}
	
	void Queue::OnSimulationLockGranted( Activity::Ptr pActivity )
	{
		PRINTEXCEPTION_AND_ABORT
		(
			pActivity->simulationLockGranted();
		);
	}

}