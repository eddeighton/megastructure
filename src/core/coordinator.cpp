
#include "megastructure/coordinator.hpp"

#include "zmq.h"

#include <iterator>
#include <sstream>

namespace megastructure
{

std::string version()
{
	return "1.0.0.0";
}

bool send( void* pSocket, const std::string& str )
{
	bool bSent = false;
	
	zmq_msg_t msg;
	int rc = zmq_msg_init_size( &msg, str.size() );
	std::copy( str.begin(), str.end(), reinterpret_cast< char* >( zmq_msg_data( &msg ) ) );
	const int iResult = zmq_sendmsg( pSocket, &msg, 0 );
	if( iResult > 0 ) 
	{
		if( str.size() == iResult )
			bSent = true;
		else
			throw std::runtime_error( "Incomplete message sent" );
	}
	else if( iResult == -1 )
	{
		switch( iResult )
		{
			case EAGAIN:
			case ENOTSUP:
			case EFSM:
			case ETERM:
			case ENOTSOCK:
			case EINTR:
			case EFAULT:
			//case ECANTROUTE:
			default:
				{
					std::ostringstream os;
					os << "Error: " << errno;
					throw std::runtime_error( os.str() );
				}
				break;
		}
	}
	
	
	return bSent;
}

bool recv( void* pSocket, std::string& str )
{
	bool bReceived = false;
	
	zmq_msg_t msg;
	{
		int rc = zmq_msg_init( &msg );
	}
	
	{
		const int rc = zmq_recvmsg( pSocket, &msg, 0 );
		if( rc > 0 )
		{
			const void* pData = zmq_msg_data( &msg );
			const std::size_t szSize = zmq_msg_size( &msg );
			
			const char* pStr = reinterpret_cast< const char* >( pData );
			str.clear();
			std::copy( pStr, pStr + szSize, std::back_inserter( str ) );
			
			bReceived = true;
			
		}
		else if( rc == -1 )
		{
			switch( errno )
			{
				case EAGAIN:
				{
					std::ostringstream os;
					os << "EAGAIN: " << errno;
					throw std::runtime_error( os.str() );
				}
				case ENOTSUP:
				{
					std::ostringstream os;
					os << "ENOTSUP: " << errno;
					throw std::runtime_error( os.str() );
				}
				case EFSM:
				{
					/*std::ostringstream os;
					os << "EFSM: " << errno;
					throw std::runtime_error( os.str() );*/
					break;
				}
				case ETERM:
				{
					std::ostringstream os;
					os << "ETERM: " << errno;
					throw std::runtime_error( os.str() );
				}
				case ENOTSOCK:
				{
					std::ostringstream os;
					os << "ENOTSOCK: " << errno;
					throw std::runtime_error( os.str() );
				}
				case EINTR:
				{
					std::ostringstream os;
					os << "EINTR: " << errno;
					throw std::runtime_error( os.str() );
				}
				case EFAULT:
				{
					std::ostringstream os;
					os << "EFAULT: " << errno;
					throw std::runtime_error( os.str() );
				}
				default:
				{
					std::ostringstream os;
					os << "Unknown error code: " << errno;
					throw std::runtime_error( os.str() );
				}
			}
		}
	}
	
	zmq_msg_close( &msg );
	
	return bReceived;
}


Master::Master()
{
	m_pContext 		= zmq_ctx_new();
	m_pSocket 		= zmq_socket( m_pContext, ZMQ_REP );
	
	{
		int64_t zmqLinger = 0;
		zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
	}
	
	int result = zmq_bind( m_pSocket, "tcp://*:5555" );
	if( result != 0 )
	{
		throw std::runtime_error( "Failed to bind to tcp://*:5555" );
	}
}

Master::~Master()
{
	zmq_ctx_term( m_pContext );
	zmq_ctx_shutdown( m_pContext );
}


bool Master::poll( std::string& str )
{
	if( ::megastructure::recv( m_pSocket, str ) )
	{
		::megastructure::send( m_pSocket, "Success" );
		return true;
	}
	return false;
}



Slave::Slave()
{
	m_pContext 		= zmq_ctx_new();
	m_pSocket 		= zmq_socket( m_pContext, ZMQ_REQ );
	
	{
		int64_t zmqLinger = 0;
		zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
	}
	
	int result = zmq_connect( m_pSocket, "tcp://localhost:5555" );
	if( result != 0 )
	{
		throw std::runtime_error( "Failed to bind to tcp://localhost:5555" );
	}
	
}
Slave::~Slave()
{
	zmq_ctx_term( m_pContext );
	zmq_ctx_shutdown( m_pContext );
}

bool Slave::send( const std::string& str, std::string& strResponse )
{
	if( ::megastructure::send( m_pSocket, str ) )
	{
		return ::megastructure::recv( m_pSocket, strResponse );
	}
	return false;
}
		
}
