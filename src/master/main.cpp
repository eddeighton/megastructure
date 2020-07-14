

#include "activities.hpp"
#include "master.hpp"

#include "megastructure/log.hpp"

#include "common/processID.hpp"
#include "common/assert_verify.hpp"


#include <boost/program_options.hpp>

struct Args
{
	std::string port;
	boost::filesystem::path master_path;
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
			("path",  po::value< boost::filesystem::path >( &args.master_path ), "Workspace Path" )
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
	
    try 
    {
        //configure log
        auto logThreadPool = megastructure::configureLog( "master" );
    
        {
            spdlog::info( "Master started with pid:{}", Common::getProcessID() );
            Environment environment;
            
            master::Master master( environment, args.master_path, args.port );
            
            while( true )
            {
                std::string inputString;
                std::cin >> inputString;
                
                if( inputString == "help" )
                {
                    std::cout << "help:   This...\n";
                    std::cout << "test:   Test existing connections and drop inactive ones.\n";
                    std::cout << "list:   List existing connections.\n";
                    std::cout << "load:   Load a program.\n";
                    std::cout << "quit:   Quit.\n";
                }
                else if( inputString == "test" )
                {
                    master.startActivity( new master::TestClientsActivity( master ) );
                }
                else if( inputString == "list" )
                {
                    master.startActivity( new master::ListClientsActivity( master ) );
                }
                else if( inputString == "load" )
                {
                    std::string programName;
                    std::cin >> programName;
                    master.startActivity( new master::LoadProgram( master, programName ) );
                }
                else if( inputString == "quit" )
                {
                    break;
                }
            }
        }
        
        spdlog::drop_all(); 
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log configuration failed" << std::endl;
    }
	
	return 0;
}
