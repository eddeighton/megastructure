
#include "slave.hpp"
#include "activitiesHost.hpp"
#include "activitiesMaster.hpp"

#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <thread>
#include <chrono>

struct Args
{
	std::string master_ip;
	std::string master_port;
	std::string slave_port;
	std::string slave_name;
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
			("mip",    po::value< std::string >( &args.master_ip ), "Master IP Address" )
			("mport",  po::value< std::string >( &args.master_port ), "Master Port" )
			("sport",  po::value< std::string >( &args.slave_port ), "Slave Port" )
			("sname",  po::value< std::string >( &args.slave_name ), "Slave Name" )
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
		
		if( args.master_ip.empty() )
		{
			std::cout << "Master IP address not specified" << std::endl;
			return false;
		}
		
		if( args.master_port.empty() )
		{
			std::cout << "Master Port not specified" << std::endl;
			return false;
		}
		
		if( args.slave_port.empty() )
		{
			std::cout << "Slave Port not specified" << std::endl;
			return false;
		}
		
		if( args.slave_name.empty() )
		{
			std::cout << "Slave Name not specified" << std::endl;
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
		slave::Slave slave( args.master_ip, args.master_port, args.slave_port, args.slave_name );
		
		std::string str, strResponse;
		while( true )
		{
			std::cin >> str;
			
			if( str == "test" )
				slave.startActivity( new slave::TestHostsActivity( slave ) );
			else if( str == "list" )
				slave.startActivity( new slave::ListHostsActivity( slave ) );
			else if( str == "quit" )
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