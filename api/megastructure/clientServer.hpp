
#include <string>
#include <cstdint>

namespace megastructure
{

	class Client
	{
	public:
		Client( const std::string& strMasterIP, const std::string& strMasterPort );
		~Client();
		
		void send( const std::string& str );
		std::string recv();
		
	private:
		void* m_pContext;
		void* m_pSocket;
	};

	class Server
	{
	public:
		Server( const std::string& strPort );
		~Server();
		
		void send( const std::string& str, std::uint32_t uiClient );
		std::string recv( std::uint32_t& uiClient );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		
		uint32_t m_uiRoutingID;
	};

}
