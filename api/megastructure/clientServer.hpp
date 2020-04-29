
#pragma once

#include "protocol/megastructure.pb.h"

#include <string>
#include <cstdint>
#include <set>

namespace megastructure
{
	
	std::string getGlobalCoordinatorPort();

	class Client
	{
	public:
		Client( const std::string& strMasterIP, const std::string& strMasterPort );
		~Client();
		
		void stop();
		bool send( Message& message );
		bool recv_sync( Message& message, bool& bReceived );
		
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
		
		void stop();
		bool send( Message& message, std::uint32_t uiClient );
		bool recv_sync( Message& message, std::uint32_t& uiClient, bool& bReceived );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		std::int32_t m_messageID;
	};

	
}
