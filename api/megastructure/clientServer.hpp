
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
		void broadcast( const std::string& str );
		bool recv( std::string& strMsg, std::uint32_t& uiClient );
		
	private:
		void* m_pContext;
		void* m_pSocket;
		
		std::set< uint32_t> m_clients;
	};

}
