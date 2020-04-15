

#include "megastructure/coordinator.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"

#include "common/processID.hpp"

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>

#include <signal.h>

struct Args
{
	std::string port;
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


class Master
{
public:
	using ClientIDMap = std::map< std::string, std::uint32_t >;

	bool enrole( const std::string& strName, std::uint32_t clientID )
	{
		ClientIDMap::iterator iFind = m_clients.find( strName );
		if( iFind == m_clients.end() )
		{
			m_clients.insert( std::make_pair( strName, clientID ) );
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void slaveDied( const std::string& strName, std::uint32_t clientID )
	{
		ClientIDMap::iterator iFind = m_clients.find( strName );
		if( iFind != m_clients.end() )
		{
			if( iFind->second == clientID )
			{
				m_clients.erase( iFind );
			}
		}
	}
	
	const ClientIDMap& getClients() const { return m_clients; }
	
private:
	ClientIDMap m_clients;
};

class EnrollActivity : public megastructure::Activity
{
public:
	EnrollActivity( Master& master,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	Activity( queue ),
			m_master( master ),
			m_server( server )
	{
		
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		if( message.has_slavehostrequest_enroll() )
		{
			const megastructure::Message::SlaveHostRequest_Enroll& enroll =
				message.slavehostrequest_enroll();
			
			std::cout << "Enrole request from: " << uiClient << 
				" for role: " << enroll.slavename() << std::endl;
				
			if( m_master.enrole( enroll.slavename(), uiClient ) )
			{
				std::string str;
				{
					megastructure::Message message;
					megastructure::Message::HostSlaveResponse_Enroll* pEnroll =
						message.mutable_hostslaveresponse_enroll();
					pEnroll->set_success( true );
					message.SerializeToString( &str );
				}
				m_server.send( str, uiClient );
			}
			else
			{
				std::string str;
				{
					megastructure::Message message;
					megastructure::Message::HostSlaveResponse_Enroll* pEnroll =
						message.mutable_hostslaveresponse_enroll();
					pEnroll->set_success( false );
					message.SerializeToString( &str );
				}
				m_server.send( str, uiClient );
			}
			
			return true;
		}
		return false;
	}
	
private:
	Master& m_master;
	megastructure::Server& m_server;
};

class TestClientActivity : public megastructure::Activity
{
public:
	TestClientActivity( Master& master,
				megastructure::Queue& queue, 
				megastructure::Server& server, std::uint32_t clientID, const std::string& strSlaveName ) 
		:	Activity( queue ),
			m_master( master ),
			m_server( server ),
			m_clientID( clientID ),
			m_strSlaveName( strSlaveName )
	{
		
	}
	
	virtual void start()
	{
		std::cout << "TestClientActivity::start clientid: " << m_clientID << " name: " << m_strSlaveName << std::endl;
		std::string str;
		{
			megastructure::Message message;
			megastructure::Message::HostSlaveRequest_Alive* pAlive =
				message.mutable_hostslaverequest_alive();
			pAlive->set_slavename( m_strSlaveName );
			message.SerializeToString( &str );
		}
		m_server.send( str, m_clientID );
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		if( m_clientID == uiClient )
		{
			if( message.has_slavehostresponse_alive() )
			{
				const megastructure::Message::SlaveHostResponse_Alive& alive =
					message.slavehostresponse_alive();
				if( !alive.success() )
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is not alive" << std::endl;
					m_master.slaveDied( m_strSlaveName, m_clientID );
				}
				else
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is alive" << std::endl;
				}
				m_queue.activityComplete( shared_from_this() );
				return true;
			}
		}
		return false;
	}
	
private:
	Master& m_master;
	megastructure::Server& m_server;
	std::uint32_t m_clientID;
	std::string m_strSlaveName;
};

class TestClientsActivity : public megastructure::Activity
{
public:
	TestClientsActivity( Master& master,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	Activity( queue ),
			m_master( master ),
			m_server( server )
	{
		
	}
	
	virtual void start()
	{
		const Master::ClientIDMap& clients = m_master.getClients();
		for( Master::ClientIDMap::const_iterator 
			i = clients.begin(), iEnd = clients.end();
			i!=iEnd; ++i )
		{
			megastructure::Activity::Ptr pActivity( 
				new TestClientActivity( m_master, m_queue, m_server, i->second, i->first ) );
			m_activities.push_back( pActivity );
			m_queue.startActivity( pActivity );
		}
	}
	
	virtual bool activityComplete( Activity::Ptr pActivity )
	{
		megastructure::Activity::PtrList::iterator iFind = 
			std::find( m_activities.begin(), m_activities.end(), pActivity );
		if( iFind == m_activities.end() )
		{
			return false;
		}
		else
		{
			m_activities.erase( iFind );
			if( m_activities.empty() )
			{
				std::cout << "TestClientsActivity completed" << std::endl;
				m_queue.activityComplete( shared_from_this() );
			}
			return true;
		}
	}
	
private:
	Master& m_master;
	megastructure::Server& m_server;
	megastructure::Activity::PtrList m_activities;
};

int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
	
	{
		Master master;
		megastructure::Queue queue;
		megastructure::Server server( args.port );
		
		std::thread zeromqserver( [ &server, &queue ]()
		{
			megastructure::readServer( server, queue );
		});
		
		megastructure::Activity::Ptr pActivity( 
			new EnrollActivity( master, queue, server ) );
		queue.startActivity( pActivity );
		
		while( true )
		{
			std::string inputString;
			std::cin >> inputString;
			
			if( inputString == "test" )
			{
				//test client connections
				std::cout << "Starting Alive Test" << std::endl;
				megastructure::Activity::Ptr pActivity( 
					new TestClientsActivity( master, queue, server ) );
				queue.startActivity( pActivity );
			}
			else
			{
				break;
			}
		}
		
		queue.stop();
	}
	
	std::cout << "io works!" << std::endl;
	return 0;
}
