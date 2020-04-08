

#include "megastructure/coordinator.hpp"

#include <iostream>
#include <chrono>
#include <thread>

#include <signal.h>

int main( int argc, const char* argv[] )
{
	try
	{
		std::cout << "Master: " << megastructure::version() << std::endl;
		
		/*signal( SIGINT, [](int)
			{ 
				std::cout << "interupted" << std::endl;
				std::abort(); 
			} );*/
			
		
		megastructure::Master master;
		std::string str;
		while( true )
		{
			if( master.poll( str ) )
			{
				std::cout << str << std::endl;
				using namespace std::chrono_literals;
				std::this_thread::sleep_for( 0.5s );
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
