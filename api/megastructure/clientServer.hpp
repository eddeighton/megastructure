
#pragma once

#include "protocol/megastructure.pb.h"

#include <string>
#include <sstream>
#include <cstdint>
#include <set>

namespace megastructure
{
	std::string getGlobalCoordinatorPort();
	
	class SocketName
	{
	public:
		virtual ~SocketName() {}
		virtual std::string getName() const = 0;
	};
	
	class TCPLocalSocketName : public SocketName
	{
	public:
		TCPLocalSocketName( const std::string& strPort )
			:	m_strPort( strPort )
		{
		}
		std::string getName() const
		{
			std::ostringstream os;
			os << "tcp://*:" << m_strPort;
			return os.str();
		}
	private:
		std::string m_strPort;
	};
	
	class TCPRemoteSocketName : public SocketName
	{
	public:
		TCPRemoteSocketName( const std::string& strMasterIP, const std::string& strPort )
			:	m_strIP( strMasterIP ),
				m_strPort( strPort )
		{
		}
		std::string getName() const
		{
			std::ostringstream os;
			os << "tcp://" << m_strIP << ':' << m_strPort;
			return os.str();
		}
	private:
		std::string m_strIP;
		std::string m_strPort;
	};

	class Client
	{
	public:
		Client( const SocketName& socketName );
		~Client();
		
		void stop();
		bool send( const Message& message );
		bool recv_sync( Message& message, bool& bReceived );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		std::int32_t m_messageID;
	};

	class Server
	{
	public:
		Server( const SocketName& socketName );
		~Server();
		
		void stop();
		bool send( const Message& message, std::uint32_t uiClient );
		bool recv_sync( Message& message, std::uint32_t& uiClient, bool& bReceived );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		std::int32_t m_messageID;
	};

	
}
