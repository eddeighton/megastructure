
#include "slave.hpp"
#include "activitiesHost.hpp"
#include "activitiesMaster.hpp"

#include "megastructure/mega.hpp"
#include "megastructure/log.hpp"

#include "schema/project.hpp"

#include "common/assert_verify.hpp"
#include "common/processID.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/process/environment.hpp>

#include <iostream>
#include <thread>
#include <chrono>

struct Args
{
	std::string master_ip;
	std::string master_port;
	std::string slave_mega_port;
	std::string slave_eg_port;
	boost::filesystem::path slave_path;
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
			("mip",    po::value< std::string >( &args.master_ip ), "Master IP Address" )
			("mport",  po::value< std::string >( &args.master_port ), "Master Port" )
			("spath",  po::value< boost::filesystem::path >( &args.slave_path ), "Path to workspace coordinator folder for slave" )
			("sport",  po::value< std::string >( &args.slave_mega_port )->default_value( megastructure::MEGA_PORT ), "Slave Mega Port" )
			("eport",  po::value< std::string >( &args.slave_eg_port )->default_value( megastructure::EG_PORT ), "Slave EG Port" )
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
		
		if( args.slave_path.empty() )
		{
			std::cout << "Slave Path not specified" << std::endl;
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
    
	
	try
	{
		
		boost::filesystem::path workspacePath;
		std::string strSlaveName;
		slave::getWorkspaceAndSlaveNameFromPath( 
			args.slave_path, workspacePath, strSlaveName );
            
        Environment environment( workspacePath );
            
        auto logThreadPool = megastructure::configureLog( environment.getLogFolderPath(), strSlaveName );
		
		slave::Slave slave( 
			environment,
			args.master_ip, 
			args.master_port, 
			args.slave_mega_port,
			args.slave_eg_port,
			workspacePath,
			strSlaveName );
			
		if( !slave.getEnrollment().get() )
		{
            SPDLOG_WARN( "Failed to enroll slave with master as: {} in workspace: {} program: {}",
                slave.getName(), slave.getWorkspace().string(), slave.getActiveProgramName() );
			return 0;
		}
		else
		{
            SPDLOG_INFO( "Slave successfully enrolled with master as: {} in workspace: {} program: {} pid: {}",
                slave.getName(), slave.getWorkspace().string(), slave.getActiveProgramName(), Common::getProcessID() );
		}
		
		std::string str, strResponse;
		while( true )
		{
			std::cin >> str;
			
			if( str == "test" )
				slave.startActivity( new slave::TestHostsActivity( slave ) );
			else if( str == "list" )
				slave.startActivity( new slave::ListHostsActivity( slave ) );
            else if( str == "help" )
            {
                std::cout << "help:   This...\n";
                std::cout << "test:   Test existing connections and drop inactive ones.\n";
                std::cout << "list:   List existing connections.\n";
                std::cout << "quit:   Quit.\n";
            }
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