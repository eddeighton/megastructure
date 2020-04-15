
#pragma once

#include "protocol/megastructure.pb.h"

#include <string>
#include <cstdint>
#include <set>

namespace megastructure
{

	class Client
	{
	public:
		Client( const std::string& strMasterIP, const std::string& strMasterPort );
		~Client();
		
		bool send( Message& message );
		bool recv_sync( Message& message );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		std::int32_t m_messageID;
	};

	class Server
	{
	public:
		Server( const std::string& strPort );
		~Server();
		
		bool send( Message& message, std::uint32_t uiClient );
		bool recv_sync( Message& message, std::uint32_t& uiClient );
	private:
		void* m_pContext;
		void* m_pSocket;
		std::int32_t m_messageID;
	};

	
}
