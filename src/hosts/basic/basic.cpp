
#include <iostream>
#include <chrono>
#include <thread>
//#include <signal.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include "protocol/protocol_helpers.hpp"

#include "megastructure/component.hpp"
#include "megastructure/mega.hpp"
#include "egcomponent/egcomponent.hpp"


struct Args
{
	std::string slave_mega_port;
	std::string slave_eg_port;
    std::string programName;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("sport", po::value< std::string >( &args.slave_mega_port )->default_value( megastructure::MEGA_PORT ), "Slave Mega Port" )
			("eport", po::value< std::string >( &args.slave_eg_port )->default_value( megastructure::EG_PORT ), "Slave EG Port" )
            ("name", po::value< std::string >( &args.programName ), "Alternative name for program to be known by instead of default actual process name" )
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
        if( args.programName.empty() )
        {
            args.programName = megastructure::getHostProgramName();
        }
		
		std::future< std::string > inputStringFuture =
			std::async( std::launch::async, readInput );
			
		Environment environment;
		
		megastructure::Component component( 
			environment,
			args.slave_mega_port,
			args.slave_eg_port,
			args.programName,
            nullptr );
            
        SPDLOG_INFO( "Host: {} pid: {}", args.programName, Common::getProcessID() );
			
		while( true )
		{
			using namespace std::chrono_literals;
			const std::future_status status =
				inputStringFuture.wait_for( 100ms );
			if( status == std::future_status::deferred ||
				status == std::future_status::ready )
			{
				std::string strInput = inputStringFuture.get();
				if( strInput == "quit" )
				{
					break;
				}
                else if( strInput == "help" )
                {
                    std::cout << "help - this message" << std::endl;
                    std::cout << "quit - shutdown this host" << std::endl;
                }
                else if( strInput == "" )
                {
                    //do nothing
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
