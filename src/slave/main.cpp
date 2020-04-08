
#include "megastructure/coordinator.hpp"

#include <iostream>

#include <signal.h>

int main( int argc, const char* argv[] )
{
	try
	{
		std::cout << "Slave: " << megastructure::version() << std::endl;
		
		/*signal( SIGINT, [](int)
			{ 
				std::cout << "interupted" << std::endl;
				std::abort(); 
			} );*/
			
		megastructure::Slave slave;
			
		std::string str, strResponse;
		while( true )
		{
			std::cin >> str;
			if( slave.send( str, strResponse ) )
			{
				std::cout << strResponse << std::endl;
			}
			else
			{
				std::cout << "Message failed" << std::endl;
			}
		}
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	return 0;
}
