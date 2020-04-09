

#include "megastructure/clientServer.hpp"


#include "zmq.h"

#include <string>
#include <sstream>

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
		if( result != 0 )
		{
			throw std::runtime_error( "Failed to connect to " + strMasterEndpoint );
		}
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
		//assert (rc == 0);
		char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
		std::copy( str.begin(), str.end(), pData );
		rc = zmq_msg_send( &msg, m_pSocket, 0); 
		//assert(rc == 6);
	}
	
	std::string Client::recv()
	{
		/* Create an empty ØMQ message */
		zmq_msg_t msg;
		int rc = zmq_msg_init( &msg );
		//assert (rc == 0);
		/* Block until a message is available to be received from socket */
		rc = zmq_msg_recv( &msg, m_pSocket, 0 );
		//assert (rc != -1);
		
		const std::size_t szSize = zmq_msg_size( &msg );
		const char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ) );
		const std::string result( pData, pData + szSize );
		
		/* Release message */ 
		zmq_msg_close (&msg);

		
		return result;
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
		
		int result = zmq_bind( m_pSocket, localAddress.c_str() );
		if( result != 0 )
		{
			throw std::runtime_error( "Failed to bind to " + localAddress );
		}
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
		//assert (rc == 0);
		
		char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ));
		std::copy( str.begin(), str.end(), pData );
		
		rc = zmq_msg_set_routing_id( &msg, uiClient );
		
		rc = zmq_msg_send( &msg, m_pSocket, 0); 
		//assert(rc == 6);
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
		//assert (rc == 0);
		/* Block until a message is available to be received from socket */
		rc = zmq_msg_recv( &msg, m_pSocket, ZMQ_DONTWAIT );
		//assert (rc != -1);
		
		if( rc > 0 )
		{
			uiClient = zmq_msg_routing_id( &msg );
			
			m_clients.insert( uiClient );
			
			const std::size_t szSize = zmq_msg_size( &msg );
			const char* pData = reinterpret_cast< char* >( zmq_msg_data( &msg ) );
			strMsg = std::string( pData, pData + szSize );
			
			/* Release message */ 
			
			bResult = true;
		}
		
		zmq_msg_close( &msg );
		
		return bResult;
	}
}