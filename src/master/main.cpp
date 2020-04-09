

#include "megastructure/coordinator.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <signal.h>

struct Args
{
	std::string ip;
	std::string port;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("help", "produce help message")
			("ip",    po::value< std::string >( &args.ip ), "IP Address" )
			("port",  po::value< std::string >( &args.port ), "Port" )
		;

		po::positional_options_description p;
		p.add( "filters", -1 );

		po::store( po::command_line_parser( argc, argv).
					options( options ).
					positional( p ).run(),
					variables );
		po::notify( variables );
		
		if( variables.count("help") )
		{
			std::cout << options << "\n";
			return false;
		}
		
		if( args.ip.empty() )
		{
			std::cout << "Missing IP address" << std::endl;
			return false;
		}
		
		if( args.port.empty() )
		{
			std::cout << "Port" << std::endl;
			return false;
		}

	}
	catch( std::exception& e )
	{
		std::cout << "Invalid input. Type '--help' for options\n" << e.what() << "\n";
		return false;
	}
	return true;
}

int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
	
	try
	{
		std::cout << "Master: " << megastructure::version() << std::endl;
		
		/*signal( SIGINT, [](int)
			{ 
				std::cout << "interupted" << std::endl;
				std::abort(); 
			} );*/
			
		
		megastructure::Master master( args.ip, args.port );
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
