

#include "activities.hpp"
#include "master.hpp"

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>

struct Args
{
	std::string port;
	bool bWait;
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
			("wait",   po::bool_switch( &args.bWait ), "Wait at startup for attaching a debugger" )
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
			std::cout << "Port not specified" << std::endl;
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
	
	if( args.bWait )
	{
		char c;
		std::cin >> c;
	}
	
	{
		master::Master master( args.port );
		
		while( true )
		{
			std::string inputString;
			std::cin >> inputString;
			
			if( inputString == "test" )
			{
				master.startActivity( new master::TestClientsActivity( master ) );
			}
			else if( inputString == "list" )
			{
				master.startActivity( new master::ListClientsActivity( master ) );
			}
			else if( inputString == "quit" )
			{
				break;
			}
		}
	}
	
	std::cout << "io works!" << std::endl;
	return 0;
}
