

//#include "megastructure/coordinator.hpp"
#include "megastructure/clientServer.hpp"

#include "common/processID.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <signal.h>

struct Args
{
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
	
	using namespace std::chrono_literals;
				
	try
	{
		std::cout << "Master: " << Common::getProcessID() << std::endl;
		
		megastructure::Server server( args.port );
		std::string str;
		while( true )
		{
			std::uint32_t uiClient = 0;
			if( server.recv( str, uiClient ) )
			{
				std::cout << "\n" << str << std::endl;
				
				{
					server.send( "Got it!", uiClient );
					std::this_thread::sleep_for( 0.1s );
				}
				
				{
					std::ostringstream os;
					os << "Got: " << str << " from: " << uiClient;
					server.broadcast( os.str() );
					std::this_thread::sleep_for( 0.1s );
				}
			}
			
			std::this_thread::sleep_for( 1s );
			std::cout << ".";
			std::cout.flush();
		}
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	
	return 0;
}
