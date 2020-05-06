

#include "megastructure/clientServer.hpp"

#include "common/assert_verify.hpp"

#include "zmq.h"

#include <string>
#include <sstream>
#include <cstdlib>

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
				THROW_RTE( "zmq_msg_recv returned: The 0MQ context associated with the specified socket was terminated." );
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
	/*
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
	}*/
	
	class ZMQMsg
	{
	public:
		/*ZMQMsg( const std::string& str )
		{
			bRequiresClose = false;
			
			int rc = zmq_msg_init_size( &msg, str.size() );
			VERIFY_RTE_MSG( rc == 0, "zmq_msg_init_size failed" );
			
			char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
			std::copy( str.begin(), str.end(), pData );
		}*/
		
		ZMQMsg( const megastructure::Message& message ) 
		{
			const std::size_t szSize = message.ByteSizeLong();
			
			int rc = zmq_msg_init_size( &msg, szSize );
			VERIFY_RTE_MSG( rc == 0, "zmq_msg_init_size failed" );
			
			char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
			message.SerializeToArray( pData, szSize );
		}
		
		ZMQMsg( const megastructure::Message& message, std::uint32_t uiClient ) 
		{
			const std::size_t szSize = message.ByteSizeLong();
			
			int rc = zmq_msg_init_size( &msg, szSize );
			VERIFY_RTE_MSG( rc == 0, "zmq_msg_init_size failed" );
			
			char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
			message.SerializeToArray( pData, szSize );
			
			rc = zmq_msg_set_routing_id( &msg, uiClient );
			VERIFY_RTE_MSG( rc != -1, "zmq_msg_set_routing_id failed" );
		}
		
		ZMQMsg()
		{
			int rc = zmq_msg_init( &msg );
			VERIFY_RTE_MSG( rc == 0, "zmq_msg_init failed" );
			//bRequiresClose = true;
		}
		
		~ZMQMsg()
		{
			//if( bRequiresClose )
			{
				/* Release message */ 
				zmq_msg_close( &msg );
			}
		}
		inline void readMessage( megastructure::Message& message )
		{
			const std::size_t szSize = zmq_msg_size( &msg );
			const char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ) );
			if( !message.ParseFromArray( pData, szSize ) )
			{
				throw std::runtime_error( 
					"Failed to parse received message: " + std::string( pData, pData + szSize ) );
			}
		}
		
		inline void readMessage( megastructure::Message& message, std::uint32_t& uiClient )
		{
			uiClient = zmq_msg_routing_id( &msg );
			VERIFY_RTE_MSG( uiClient != 0, "zmq_msg_routing_id failed" );
			
			readMessage( message );
		}
		
		inline zmq_msg_t* get() { return &msg; }
		
	private:
		zmq_msg_t msg;
		//bool bRequiresClose = false;
	};
}

namespace megastructure
{

	Client::Client( const SocketName& socketName )
		:	m_messageID( 1 ),
			m_pSocket( nullptr ),
			m_pContext( nullptr )
	{
		m_pContext 		= zmq_ctx_new();
		m_pSocket 		= zmq_socket( m_pContext, ZMQ_CLIENT );
		
		{
			int64_t zmqLinger = 0;
			zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
		}
		
		const std::string strSlaveEndpoint = socketName.getName();
		std::cout << "Connecting to: " << strSlaveEndpoint << std::endl;
		int result = zmq_connect( m_pSocket, strSlaveEndpoint.c_str() );
		VERIFY_RTE_MSG( result == 0, "Failed to connect to: " << strSlaveEndpoint );
	}
	
	Client::~Client()
	{
        stop();
	}
	
	void Client::stop()
	{
		if( m_pSocket != nullptr )
		{
			zmq_close( m_pSocket );
			m_pSocket = nullptr;
		}
		
		if( m_pContext != nullptr )
		{
			zmq_ctx_shutdown( m_pContext );
			m_pContext = nullptr;
		}
	}
	
	bool Client::send( const Message& message )
	{
		//message.set_id( m_messageID++ );
		
		ZMQMsg msg( message );
		
		const int rc = zmq_msg_send( msg.get(), m_pSocket, 0 ); 
		if( rc == -1 )
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
		return true;
	}
	
	bool Client::recv_sync( Message& message, bool& bReceived )
	{
		/* Create an empty ØMQ message */
		ZMQMsg msg;
		
		/* Block until a message is available to be received from socket */
		const int rc = zmq_msg_recv( msg.get(), m_pSocket, 0 );
		
		//test if connection was broken
		if( rc < 0 && errno == ETERM )
		{
			return false;
		}
		
		if( rc >= 0 )
		{
			msg.readMessage( message );
			bReceived = true;
		}
		else //if( errno != EAGAIN )
		{
			throwReceiveError();
		}
		
		return true;
	}
	
	////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////
	Server::Server( const SocketName& socketName )
		:	m_messageID( 1 ),
			m_pSocket( nullptr ),
			m_pContext( nullptr )
			
	{
		m_pContext 		= zmq_ctx_new();
		m_pSocket 		= zmq_socket( m_pContext, ZMQ_SERVER );
		
		/*{
			int64_t zmqLinger = 0;
			zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
		}*/
		
		const std::string localAddress = socketName.getName();
		int rc = zmq_bind( m_pSocket, localAddress.c_str() );
		VERIFY_RTE_MSG( rc == 0, "Failed to bind to: " << localAddress );
	}
	
	Server::~Server()
	{
        stop();
	}
	
	void Server::stop()
	{
		if( m_pSocket != nullptr )
		{
			zmq_close( m_pSocket );
			m_pSocket = nullptr;
		}
		
		if( m_pContext != nullptr )
		{
			zmq_ctx_term( m_pContext );
			//zmq_ctx_shutdown( m_pContext );
			m_pContext = nullptr;
		}
	}
	
	bool Server::send( const Message& message, std::uint32_t uiClient )
	{
		//message.set_id( m_messageID++ );
		
		ZMQMsg msg( message, uiClient );
		
		const int rc = zmq_msg_send( msg.get(), m_pSocket, 0); 
		if( rc == -1 )
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
					
				//case 110://ERROR_OPEN_FAILED: //windows error code for "The system cannot open the device or file specified."
				//	return false;
					
				default:
					return false;
					//THROW_RTE( "zmq_msg_send returned unknown error code: " << errno );
			}
		}
		return true;
	}
	
	bool Server::recv_sync( Message& message, std::uint32_t& uiClient, bool& bReceived )
	{
		// Create an empty ØMQ message 
		ZMQMsg msg;
		
		// Block until a message is available to be received from socket 
		const int rc = zmq_msg_recv( msg.get(), m_pSocket, 0 );
		
		//test if connection was broken
		if( rc < 0 && errno == ETERM )
		{
			return false;
		}
		
		if( rc >= 0 )
		{
			msg.readMessage( message, uiClient );
			bReceived = true;
		}
		else //if( errno != EAGAIN )
		{
			throwReceiveError();
		}
		
		return true;
	}
}