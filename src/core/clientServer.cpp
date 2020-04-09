

#include "megastructure/clientServer.hpp"

#include "common/assert_verify.hpp"

#include "zmq.h"

#include <string>
#include <sstream>

namespace
{
	void throwReceiveError()
	{
		switch( errno )
		{
			case EAGAIN:
				THROW_RTE( "zmq_msg_recv returned: Non-blocking mode was requested and no messages are available at the moment." );
			case ENOTSUP:
				THROW_RTE( "zmq_msg_recv returned: The zmq_msg_recv() operation is not supported by this socket type." );
			case EFSM:
				THROW_RTE( "zmq_msg_recv returned: The zmq_msg_recv() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. This error may occur with socket types that switch between several states, such as ZMQ_REP. See the messaging patterns section of zmq_socket(3) for more information." );
			case ETERM:
				THROW_RTE( "zmq_msg_recv returned: The ØMQ context associated with the specified socket was terminated." );
			case ENOTSOCK:
				THROW_RTE( "zmq_msg_recv returned: The provided socket was invalid." );
			case EINTR:
				THROW_RTE( "zmq_msg_recv returned: The operation was interrupted by delivery of a signal before a message was available." );
			case EFAULT:
				THROW_RTE( "zmq_msg_recv returned: The message passed to the function was invalid." );
			default:
				THROW_RTE( "zmq_msg_recv returned: unknown error: " << errno );
		}
	}
	
	void throwSendError()
	{
		switch( errno )
		{
			case EAGAIN:
				THROW_RTE( "zmq_msg_send returned : Non-blocking mode was requested and the message cannot be sent at the moment." );
			case ENOTSUP:
				THROW_RTE( "zmq_msg_send returned : The zmq_msg_send() operation is not supported by this socket type." );
			case EFSM:
				THROW_RTE( "zmq_msg_send returned : The zmq_msg_send() operation cannot be performed on this socket at the moment due to the socket not being in the appropriate state. This error may occur with socket types that switch between several states, such as ZMQ_REP. See the messaging patterns section of zmq_socket(3) for more information." );
			case ETERM:
				THROW_RTE( "zmq_msg_send returned : The ØMQ context associated with the specified socket was terminated." );
			case ENOTSOCK:
				THROW_RTE( "zmq_msg_send returned : The provided socket was invalid." );
			case EINTR:
				THROW_RTE( "zmq_msg_send returned : The operation was interrupted by delivery of a signal before the message was sent." );
			case EFAULT:
				THROW_RTE( "zmq_msg_send returned: The message passed to the function was invalid." );
			default:
				THROW_RTE( "zmq_msg_send returned unknown error code: " << errno );
		}
	}
}

namespace megastructure
{

	Client::Client( const std::string& strMasterIP, const std::string& strMasterPort )
	{
		m_pContext 		= zmq_ctx_new();
		m_pSocket 		= zmq_socket( m_pContext, ZMQ_CLIENT );
		
		{
			int64_t zmqLinger = 0;
			zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
		}
		
		std::string strMasterEndpoint;
		{
			std::ostringstream os; //"tcp://localhost:5555" 
			os << "tcp://" << strMasterIP << ':' << strMasterPort;
			strMasterEndpoint = os.str();
		}
		
		int result = zmq_connect( m_pSocket, strMasterEndpoint.c_str() );
		VERIFY_RTE_MSG( result == 0, "Failed to connect to: " << strMasterEndpoint );
	}
	
	Client::~Client()
	{
		zmq_ctx_term( m_pContext );
		zmq_ctx_shutdown( m_pContext );
	}
	
	void Client::send( const std::string& str )
	{
		zmq_msg_t msg;
		int rc = zmq_msg_init_size( &msg, str.size() );
		VERIFY_RTE_MSG( rc == 0, "zmq_msg_init_size failed" );
		
		char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
		std::copy( str.begin(), str.end(), pData );
		rc = zmq_msg_send( &msg, m_pSocket, 0 ); 
		if( rc == -1 )
		{
			throwSendError();
		}
		VERIFY_RTE_MSG( rc == str.size(), "zmq_msg_send retuned incorrect size" );
	}
	
	bool Client::recv( std::string& str )
	{
		bool bReceived = false;
		
		/* Create an empty ØMQ message */
		zmq_msg_t msg;
		int rc = zmq_msg_init( &msg );
		VERIFY_RTE_MSG( rc == 0, "zmq_msg_init failed" );
		
		/* Block until a message is available to be received from socket */
		rc = zmq_msg_recv( &msg, m_pSocket, ZMQ_DONTWAIT );
		if( rc >= 0 )
		{
			bReceived = true;
			const std::size_t szSize = zmq_msg_size( &msg );
			const char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ) );
			str.assign( pData, szSize );
		}
		else if( errno != EAGAIN )
		{
			throwReceiveError();
		}
		
		/* Release message */ 
		zmq_msg_close( &msg );
		
		return bReceived;
	}
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	Server::Server( const std::string& strPort )
	{
		m_pContext 		= zmq_ctx_new();
		m_pSocket 		= zmq_socket( m_pContext, ZMQ_SERVER );
		
		{
			int64_t zmqLinger = 0;
			zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
		}
		
		std::string localAddress;
		{
			std::ostringstream os; //"tcp://*:5555"
			os << "tcp://*" << ':' << strPort;
			localAddress = os.str();
		}
		
		int rc = zmq_bind( m_pSocket, localAddress.c_str() );
		VERIFY_RTE_MSG( rc == 0, "Failed to bind to: " << localAddress );
	}
	
	Server::~Server()
	{
		zmq_ctx_term( m_pContext );
		zmq_ctx_shutdown( m_pContext );
	}
	
	void Server::send( const std::string& str, std::uint32_t uiClient )
	{
		zmq_msg_t msg;
		int rc = zmq_msg_init_size( &msg, str.size() );
		VERIFY_RTE_MSG(rc == 0, "zmq_msg_init_size failed" );
		
		char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
		std::copy( str.begin(), str.end(), pData );
		
		rc = zmq_msg_set_routing_id( &msg, uiClient );
		VERIFY_RTE_MSG( rc != -1, "zmq_msg_set_routing_id failed" );
		
		rc = zmq_msg_send( &msg, m_pSocket, 0); 
		if( rc == -1 )
		{
			throwSendError();
		}
		VERIFY_RTE_MSG( rc == str.size(), "zmq_msg_send retuned incorrect size" );
	}
	
	void Server::broadcast( const std::string& str )
	{
		for( std::uint32_t ui : m_clients )
		{
			send( str, ui );
		}
	}
	
	bool Server::recv( std::string& strMsg, std::uint32_t& uiClient )
	{
		bool bResult = false;
		
		/* Create an empty ØMQ message */
		zmq_msg_t msg;
		int rc = zmq_msg_init( &msg );
		VERIFY_RTE_MSG( rc == 0, "zmq_msg_init failed" );
		
		/* Block until a message is available to be received from socket */
		rc = zmq_msg_recv( &msg, m_pSocket, ZMQ_DONTWAIT );
		if( rc >= 0 )
		{
			uiClient = zmq_msg_routing_id( &msg );
			
			m_clients.insert( uiClient );
			
			const std::size_t szSize = zmq_msg_size( &msg );
			const char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ) );
			strMsg.assign( pData, pData + szSize );
			
			bResult = true;
		}
		else if( errno != EAGAIN )
		{
			throwReceiveError();
		}
		
		/* Release message */ 
		zmq_msg_close( &msg );
		
		return bResult;
	}
}