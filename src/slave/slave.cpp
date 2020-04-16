
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

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
		using namespace megastructure;
		Message message;
		{
			Message::SMQ_Enroll* pEnroll =
				message.mutable_smq_enroll();
			pEnroll->set_slavename( m_name );
		}
		m_client.send( message );
	}
	
	virtual bool serverMessage( const megastructure::Message& message )
	{
		if( message.has_mss_enroll() )
		{
			std::cout << "EnrollActivity Got response: " << 
				message.mss_enroll().success() << std::endl;
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
		if( message.has_msq_alive() )
		{
			using namespace megastructure;
			
			const Message::MSQ_Alive& alive = 
				message.msq_alive();
				
			Message response;
			{
				Message::SMS_Alive* pAlive =
					response.mutable_sms_alive();
				
				if( alive.slavename() == m_name )
				{
					pAlive->set_success( true );
					std::cout << "Got alive test request. Responded true."  << std::endl;
				}
				else
				{
					pAlive->set_success( false );
					std::cout << "Got alive test request. Responded false."  << std::endl;
				}
			}
			
			m_client.send( response );
				
			return true;
		}
		return false;
	}
	
private:
	megastructure::Client& m_client;
	std::string m_name;
};


class HostEnrollActivity : public megastructure::Activity
{
public:
	HostEnrollActivity( megastructure::ClientMap& clients,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	Activity( queue ),
			m_clients( clients ),
			m_server( server )
	{
		
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		using namespace megastructure;
		
		if( message.has_hcq_enroll() )
		{
			const Message::HCQ_Enroll& enroll = message.hcq_enroll();
			
			std::cout << "Enroll request from: " << uiClient << 
				" for role: " << enroll.processname() << std::endl;
				
			if( m_clients.enroll( enroll.processname(), uiClient ) )
			{
				Message message;
				{
					Message::CHS_Enroll* pEnroll = message.mutable_chs_enroll();
					pEnroll->set_success( true );
					pEnroll->set_hostpath( "/somewhere" );
					pEnroll->set_program( "theProgram" );
				}
				if( !m_server.send( message, uiClient ) )
				{
					m_clients.removeClient( uiClient );
				}
			}
			else 
			{
				std::uint32_t uiExisting;
				/*if( m_master.getClientID( enroll.slavename(), uiExisting ) )
				{
					std::cout << "Enroll attempting for: " << enroll.slavename() << " which has existing client of: " << uiExisting << std::endl;
					std::shared_ptr< TestClientActivity > pTest = 
						std::make_shared< TestClientActivity >( m_master, m_queue, m_server, uiExisting, enroll.slavename() );
					m_queue.startActivity( pTest );
					m_testsMap.insert( std::make_pair( pTest, uiClient ) );
					return true;
				}
				else*/
				{
					std::cout << "Enroll denied for: " << enroll.processname() << " for client: " << uiClient << std::endl;
					if( !m_server.send( mss_enroll( false ), uiClient ) )
					{
						m_clients.removeClient( uiClient );
					}
				}
			}
			
			return true;
		}
		return false;
	}
	/*
	virtual bool activityComplete( Activity::Ptr pActivity )
	{
		using namespace megastructure;
		if( std::shared_ptr< TestClientActivity > pTest = 
				std::dynamic_pointer_cast< TestClientActivity >( pActivity ) )
		{
			std::map< std::shared_ptr< TestClientActivity >, std::uint32_t >::iterator 
				iFind = m_testsMap.find( pTest );
			VERIFY_RTE( iFind != m_testsMap.end() );
			if( iFind != m_testsMap.end() )
			{
				const std::uint32_t testedID = pTest->getClientID();
				const std::uint32_t clientID = iFind->second;
				m_testsMap.erase( iFind );
				if( pTest->isAlive() )
				{
					std::cout << "Existing client: " << clientID << " is alive as: " << pTest->getName() << std::endl;
					//existing client is alive so nothing we can do...
					if( !m_server.send( mss_enroll( false ), clientID ) )
					{
						m_master.removeClient( clientID );
					}
				}
				else
				{
					std::cout << "Existing client: " << testedID << " is not alive so allowing enrollment of new client: " << 
						clientID << " as: " << pTest->getName() << std::endl;
					//testing the existing client indicated it was actually dead so can enroll the new one
					if( m_master.enroll( pTest->getName(), clientID ) )
					{
						if( !m_server.send( mss_enroll( true ), clientID ) )
						{
							m_master.removeClient( clientID );
						}
					}
					else
					{
						std::cout << "Enroll denied after retry for: " << pTest->getName() << " for client: " << clientID << std::endl;
						if( !m_server.send( mss_enroll( false ), clientID ) )
						{
							m_master.removeClient( clientID );
						}
					}
				}
				return true;
			}
		}
		return false;
	}*/
private:
	megastructure::ClientMap& m_clients;
	megastructure::Server& m_server;
	//std::map< std::shared_ptr< TestClientActivity >, std::uint32_t > m_testsMap;
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
		megastructure::ClientMap clients;
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
		{
			megastructure::Activity::Ptr pActivity( 
				new HostEnrollActivity( clients, queue, server ) );
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
