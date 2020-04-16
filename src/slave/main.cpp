
#include "megastructure/clientServer.hpp"
#include "protocol/megastructure.pb.h"
#include "megastructure/queue.hpp"

//#include "zmq.h"

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


class EnrollActivity : public megastructure::Activity
{
public:
	EnrollActivity( 
				megastructure::Queue& queue, 
				megastructure::Client& client,
				const std::string& name ) 
		:	Activity( queue ),
			m_client( client ),
			m_name( name )
	{
		
	}
	
	virtual void start()
	{
		std::cout << "EnrollActivity started" << std::endl;
		
		megastructure::Message message;
		{
			megastructure::Message::SlaveHostRequest_Enroll* pEnroll =
				message.mutable_slavehostrequest_enroll();
			pEnroll->set_slavename( m_name );
		}
		m_client.send( message );
	}
	
	virtual bool serverMessage( const megastructure::Message& message )
	{
		if( message.has_hostslaveresponse_enroll() )
		{
			std::cout << "EnrollActivity Got response: " << 
				message.hostslaveresponse_enroll().success() << std::endl;
			m_queue.activityComplete( shared_from_this() );
			return true;
		}
		return false;
	}
	
private:
	megastructure::Client& m_client;
	std::string m_name;
};


class AliveTestActivity : public megastructure::Activity
{
public:
	AliveTestActivity( 
				megastructure::Queue& queue, 
				megastructure::Client& client,
				const std::string& name ) 
		:	Activity( queue ),
			m_client( client ),
			m_name( name )
	{
		
	}
	
	virtual bool serverMessage( const megastructure::Message& message )
	{
		if( message.has_hostslaverequest_alive() )
		{
			const megastructure::Message::HostSlaveRequest_Alive& alive = 
				message.hostslaverequest_alive();
				
			if( alive.slavename() == m_name )
			{
				megastructure::Message response;
				{
					megastructure::Message::SlaveHostResponse_Alive* pAlive =
						response.mutable_slavehostresponse_alive();
					pAlive->set_success( true );
				}
				m_client.send( response );
				std::cout << "Got alive test request. Responded true."  << std::endl;
			}
			else
			{
				megastructure::Message response;
				{
					megastructure::Message::SlaveHostResponse_Alive* pAlive =
						response.mutable_slavehostresponse_alive();
					pAlive->set_success( false );
				}
				m_client.send( response );
				std::cout << "Got alive test request. Responded false."  << std::endl;
			}
				
			return true;
		}
		return false;
	}
	
private:
	megastructure::Client& m_client;
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
		megastructure::Queue queue;
		megastructure::Client client( args.master_ip, args.master_port );
		megastructure::Server server( args.slave_port );
		
		std::thread zeromqclient( [ &client, &queue ]()
		{
			megastructure::readClient( client, queue );
		});
		
		std::thread zeromqserver( [ &server, &queue ]()
		{
			megastructure::readServer( server, queue );
		});
		
		{
			megastructure::Activity::Ptr pActivity( 
				new AliveTestActivity( queue, client, args.slave_name ) );
			queue.startActivity( pActivity );
		}
		{
			megastructure::Activity::Ptr pActivity( 
				new EnrollActivity( queue, client, args.slave_name ) );
			queue.startActivity( pActivity );
		}
			
		std::string str, strResponse;
		while( true )
		{
			std::cin >> str;
			
			if( str == "quit" )
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
