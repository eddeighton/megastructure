

#include <string>

namespace megastructure
{
	std::string version();
	
	
	
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
	};
}
