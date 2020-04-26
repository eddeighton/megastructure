
#include <iostream>
#include <chrono>
#include <thread>
#include <signal.h>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include "protocol/protocol_helpers.hpp"

#include "megastructure/component.hpp"
#include "egcomponent/egcomponent.hpp"


struct Args
{
	//boost::filesystem::path componentPath;
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
           //("component", po::value< boost::filesystem::path >( &args.componentPath ), "Component" )
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
		
		/*if( args.componentPath.empty() )
		{
			std::cout << "Missing component specification" << std::endl;
			return false;
		}*/

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
		/*HMODULE hModule = LoadLibrary( args.componentPath.string().c_str() );
		VERIFY_RTE_MSG( hModule, "Failed to load module: " << args.componentPath );
		
		typedef megastructure::EGComponent*(*GetComponentFunctionPtr)();
		GetComponentFunctionPtr pFunction = (GetComponentFunctionPtr) GetProcAddress( hModule, "GET_EG_COMPONENT" );
		VERIFY_RTE_MSG( pFunction, "Failed to find GET_EG_COMPONENT in module: " << args.componentPath );
		
		megastructure::EGComponent* pComponent = (*pFunction)();*/
		
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
		
		megastructure::Component component( 
			strSlavePort,
			megastructure::getHostProgramName() );
		
		while( true )
		{
			std::string strInput;
			std::cin >> strInput;
			
			if( strInput == "quit" )
				break;
		}
		
	}
	catch( std::exception& ex )
	{
		std::cout << "exception: " << ex.what() << std::endl;
		throw ex;
	}
	
	
	return 0;
}
