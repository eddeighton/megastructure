

#include <string>

namespace megastructure
{
	std::string version();
	
	
	
	class Master
	{
	public:
		Master();
		~Master();
		
		bool poll( std::string& str );
		
	private:
		void* m_pContext;
		void* m_pSocket;
	};
	
	class Slave
	{
	public:
		Slave();
		~Slave();
		
		bool send( const std::string& str, std::string& strResponse );
		
	private:
		void* m_pContext;
		void* m_pSocket;
	};
}
