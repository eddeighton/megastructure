
#include <iostream>
#include <chrono>
#include <thread>
//#include <signal.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include "protocol/protocol_helpers.hpp"

#include "megastructure/component.hpp"
#include "egcomponent/egcomponent.hpp"


struct Args
{
	std::string slavePort;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("port", po::value< std::string >( &args.slavePort ), "Slave Port" )
			("help", "produce help message")
		;

		po::positional_options_description p;
		p.add( "component", -1 );

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
	}
	catch( std::exception& e )
	{
		std::cout << "Invalid input. Type '--help' for options\n" << e.what() << "\n";
		return false;
	}
	return true;
}

std::string readInput()
{
	std::string str;
	std::getline( std::cin, str );
	return str;
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
		std::cout << "Host: " << 
			megastructure::getHostProgramName() << " : " << 
			Common::getProcessID() << std::endl;
			
		std::string strSlavePort = args.slavePort;
		if( args.slavePort.empty() )
			strSlavePort = megastructure::getGlobalCoordinatorPort();
		if( strSlavePort.empty() )
		{
			THROW_RTE( "Could not resolve Slave Port number - set MEGAPORT or pass port on command line" );
		}
		
		std::future< std::string > inputStringFuture =
			std::async( std::launch::async, readInput );
		
		megastructure::Component component( 
			strSlavePort,
			megastructure::getHostProgramName() );
		
		while( true )
		{
			using namespace std::chrono_literals;
			std::future_status status =
				inputStringFuture.wait_for( 100ms );
			if( status == std::future_status::deferred ||
				status == std::future_status::ready )
			{
				std::string strInput = inputStringFuture.get();
				if( strInput == "quit" )
				{
					break;
				}
				else
				{
					std::cout << "Unrecognised input: " << strInput << std::endl;
				}
				
				inputStringFuture =
					std::async( std::launch::async, readInput );
			}
			
			component.runCycle();
		}
		
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	
	return 0;
}
