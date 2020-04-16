

#include "megastructure/component.hpp"

#include "protocol/protocol_helpers.hpp"

#include "common/processID.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <chrono>
#include <thread>

#include <signal.h>

struct Args
{
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

	}
	catch( std::exception& e )
	{
		std::cout << "Invalid input. Type '--help' for options\n" << e.what() << "\n";
		return false;
	}
	return true;
}


class AliveTestActivity : public megastructure::Activity
{
public:
	AliveTestActivity( megastructure::Component& component ) 
		:	m_component( component )
	{
		
	}
	
	virtual bool serverMessage( const megastructure::Message& message )
	{
		if( message.has_chq_alive() )
		{
			using namespace megastructure;
			Message response;
			{
				Message::HCS_Alive* pAlive = response.mutable_hcs_alive();
				pAlive->set_success( true );
			}
			m_component.send( response );
			return true;
		}
		return false;
	}
	
private:
	megastructure::Component& m_component;
	std::string m_name;
};


int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
				
	try
	{
		std::cout << "Host: " << megastructure::getHostProgramName() << " : " << Common::getProcessID() << std::endl;
		
		megastructure::Component component( 
			megastructure::getGlobalCoordinatorPort(),
			megastructure::getHostProgramName() );
		
		component.startActivity( new AliveTestActivity( component ) );
		
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
