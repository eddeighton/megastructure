
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
#include "megastructure/mega.hpp"
#include "egcomponent/egcomponent.hpp"


struct Args
{
	std::string slave_mega_port = megastructure::MEGA_PORT;
	std::string slave_eg_port = megastructure::EG_PORT;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("sport", po::value< std::string >( &args.slave_mega_port ), "Slave Mega Port" )
			("eport", po::value< std::string >( &args.slave_eg_port ), "Slave EG Port" )
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

bool handleEGTest( megastructure::Component& component, const std::string& strLine )
{
	if( strLine.substr( 0, 4 ) == "read" )
	{
		std::uint32_t uiType 		= 0U;
		std::uint32_t uiInstance 	= 0U;
		
		{
			std::istringstream is( strLine.substr( 5 ) );
			is >> uiType >> uiInstance;
			std::cout << "Reading type: " << uiType << " instance: " << uiInstance << std::endl;
		}
		
		std::future< std::string > fResult =
			component.egRead( uiType, uiInstance );
		
		return true;
	}
	else if( strLine.substr( 0, 5 ) == "write" )
	{
		std::uint32_t uiType = 0U;
		std::uint32_t uiInstance = 0U;
		std::string strBuffer;
		
		{
			std::istringstream is( strLine.substr( 6 ) );
			is >> uiType >> uiInstance >> strBuffer;
			std::cout << "Reading type: " << uiType << " instance: " << uiInstance << " buffer: " << strBuffer << std::endl;
		}
		
		component.egWrite( uiType, uiInstance, strBuffer );
		
		return true;
	}
	else if( strLine.substr( 0, 4 ) == "call" )
	{
		std::uint32_t uiType = 0U;
		std::uint32_t uiInstance = 0U;
		std::string strBuffer;
		
		{
			std::istringstream is( strLine.substr( 5 ) );
			is >> uiType >> uiInstance >> strBuffer;
			std::cout << "Reading type: " << uiType << " instance: " << uiInstance << " buffer: " << strBuffer << std::endl;
		}
		
		component.egCall( uiType, uiInstance, strBuffer );
		
		return true;
	}
	
	return false;
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
		
		std::future< std::string > inputStringFuture =
			std::async( std::launch::async, readInput );
		
		megastructure::Component component( 
			args.slave_mega_port,
			args.slave_eg_port,
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
                else if( handleEGTest( component, strInput ) )
                {
					//do nothing
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
