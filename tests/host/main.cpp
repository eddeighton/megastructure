

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
		std::cout << "Host: " << Common::getProcessID() << std::endl;
		
		std::this_thread::sleep_for( 0.1s );
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	
	return 0;
}
