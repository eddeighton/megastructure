
	/*
	class Master
	{
	public:
		Master( const std::string& strIP, const std::string& strPort );
		~Master();
		
		bool poll( std::string& str );
		
	private:
		std::string m_strAddress;
		void* m_pContext;
		void* m_pSocket;
	};
	
	class Slave
	{
	public:
		Slave( const std::string& strIP, const std::string& strPort );
		~Slave();
		
		bool send( const std::string& str, std::string& strResponse );
		
	private:
		std::string m_strAddress;
		void* m_pContext;
		void* m_pSocket;
	};*/
/*
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


Master::Master( const std::string& strIP, const std::string& strPort )
{
	m_pContext 		= zmq_ctx_new();
	m_pSocket 		= zmq_socket( m_pContext, ZMQ_REP );
	
	{
		int64_t zmqLinger = 0;
		zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
	}
	
	{
		std::ostringstream os;
		os << "tcp://" << strIP << ':' << strPort;
		m_strAddress = os.str();
	}
	
	std::string localAddress;
	{
		std::ostringstream os; //"tcp://*:5555"
		os << "tcp://*" << ':' << strPort;
		localAddress = os.str();
	}
	
	//int result = zmq_bind( m_pSocket, localAddress.c_str() );
	int result = zmq_bind( m_pSocket, m_strAddress.c_str() );
	if( result != 0 )
	{
		throw std::runtime_error( "Failed to bind to " + m_strAddress );
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



Slave::Slave( const std::string& strIP, const std::string& strPort )
{
	m_pContext 		= zmq_ctx_new();
	m_pSocket 		= zmq_socket( m_pContext, ZMQ_REQ );
	
	{
		int64_t zmqLinger = 0;
		zmq_setsockopt( m_pSocket, ZMQ_LINGER, &zmqLinger, sizeof( zmqLinger ) );
	}
	
	{
		std::ostringstream os; //"tcp://localhost:5555" 
		os << "tcp://" << strIP << ':' << strPort;
		m_strAddress = os.str();
	}
	
	int result = zmq_connect( m_pSocket, m_strAddress.c_str() );
	if( result != 0 )
	{
		throw std::runtime_error( "Failed to bind to " + m_strAddress );
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
	*/	
	