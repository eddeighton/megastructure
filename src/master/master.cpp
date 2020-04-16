

#include "megastructure/coordinator.hpp"
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"

#include "protocol/protocol_helpers.hpp"

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>
#include <boost/asio.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <functional>
#include <map>

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

class TestClientActivity : public megastructure::Activity
{
public:
	TestClientActivity( megastructure::ClientMap& master,
				megastructure::Queue& queue, 
				megastructure::Server& server, std::uint32_t clientID, const std::string& strSlaveName ) 
		:	m_queue( queue ),
			m_master( master ),
			m_server( server ),
			m_clientID( clientID ),
			m_strSlaveName( strSlaveName ),
			m_bSuccess( false )
	{
		
	}
	
	virtual void start()
	{
		using namespace megastructure;
		Message message;
		{
			Message::MSQ_Alive* pAlive = message.mutable_msq_alive();
			pAlive->set_slavename( m_strSlaveName );
		}
		if( !m_server.send( message, m_clientID ) )
		{
			m_master.removeClient( m_clientID );
			m_queue.activityComplete( shared_from_this() );
		}
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		if( m_clientID == uiClient )
		{
			if( message.has_sms_alive() )
			{
				const megastructure::Message::SMS_Alive& alive =
					message.sms_alive();
				if( !alive.success() )
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is not alive" << std::endl;
					m_master.removeClient( m_clientID );
				}
				else
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strSlaveName << " is alive" << std::endl;
					m_bSuccess = true;
				}
				m_queue.activityComplete( shared_from_this() );
				return true;
			}
		}
		return false;
	}
	
	bool isAlive() const
	{
		return m_bSuccess;
	}
	
	const std::string& getName() const
	{
		return m_strSlaveName;
	}
	
	std::uint32_t getClientID() const 
	{
		return m_clientID;
	}
private:
	megastructure::Queue& m_queue;
	megastructure::ClientMap& m_master;
	megastructure::Server& m_server;
	std::uint32_t m_clientID;
	std::string m_strSlaveName;
	bool m_bSuccess;
};

class TestClientsActivity : public megastructure::Activity
{
public:
	TestClientsActivity( megastructure::ClientMap& master,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
			m_master( master ),
			m_server( server )
	{
		
	}
	
	virtual void start()
	{
		const megastructure::ClientMap::ClientIDMap& clients = m_master.getClients();
		for( megastructure::ClientMap::ClientIDMap::const_iterator 
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
				m_queue.activityComplete( shared_from_this() );
			}
			return true;
		}
	}
	
private:
	megastructure::Queue& m_queue;
	megastructure::ClientMap& m_master;
	megastructure::Server& m_server;
	megastructure::Activity::PtrList m_activities;
};

class ListClientsActivity : public megastructure::Activity
{
public:
	ListClientsActivity( megastructure::ClientMap& master,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
			m_master( master ),
			m_server( server )
	{
		
	}
	
	virtual void start()
	{
		const megastructure::ClientMap::ClientIDMap& clients = m_master.getClients();
		for( megastructure::ClientMap::ClientIDMap::const_iterator 
			i = clients.begin(), iEnd = clients.end();
			i!=iEnd; ++i )
		{
			std::cout << "Client: " << i->first << " id: " << i->second << std::endl;
		}
		m_queue.activityComplete( shared_from_this() );
	}
	
private:
	megastructure::Queue& m_queue;
	megastructure::ClientMap& m_master;
	megastructure::Server& m_server;
};

class EnrollActivity : public megastructure::Activity
{
public:
	EnrollActivity( megastructure::ClientMap& master,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
			m_master( master ),
			m_server( server )
	{
		
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		using namespace megastructure;
		
		if( message.has_smq_enroll() )
		{
			const Message::SMQ_Enroll& enroll =
				message.smq_enroll();
			
			std::cout << "Enroll request from: " << uiClient << 
				" for role: " << enroll.slavename() << std::endl;
				
			if( m_master.enroll( enroll.slavename(), uiClient ) )
			{
				if( !m_server.send( mss_enroll( true ), uiClient ) )
				{
					m_master.removeClient( uiClient );
				}
			}
			else 
			{
				std::uint32_t uiExisting;
				if( m_master.getClientID( enroll.slavename(), uiExisting ) )
				{
					std::cout << "Enroll attempting for: " << enroll.slavename() << " which has existing client of: " << uiExisting << std::endl;
					std::shared_ptr< TestClientActivity > pTest = 
						std::make_shared< TestClientActivity >( m_master, m_queue, m_server, uiExisting, enroll.slavename() );
					m_testsMap.insert( std::make_pair( pTest, uiClient ) );
					m_queue.startActivity( pTest );
					return true;
				}
				else
				{
					std::cout << "Enroll denied for: " << enroll.slavename() << " for client: " << uiClient << std::endl;
					if( !m_server.send( mss_enroll( false ), uiClient ) )
					{
						m_master.removeClient( uiClient );
					}
				}
			}
			
			return true;
		}
		return false;
	}
	
	virtual bool activityComplete( Activity::Ptr pActivity )
	{
		using namespace megastructure;
		if( std::shared_ptr< TestClientActivity > pTest = 
				std::dynamic_pointer_cast< TestClientActivity >( pActivity ) )
		{
			std::map< std::shared_ptr< TestClientActivity >, std::uint32_t >::iterator 
				iFind = m_testsMap.find( pTest );
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
	}
private:
	megastructure::Queue& m_queue;
	megastructure::ClientMap& m_master;
	megastructure::Server& m_server;
	std::map< std::shared_ptr< TestClientActivity >, std::uint32_t > m_testsMap;
};

int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
	
	{
		megastructure::ClientMap master;
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
				megastructure::Activity::Ptr pActivity( 
					new TestClientsActivity( master, queue, server ) );
				queue.startActivity( pActivity );
			}
			else if( inputString == "list" )
			{
				megastructure::Activity::Ptr pActivity( 
					new ListClientsActivity( master, queue, server ) );
				queue.startActivity( pActivity );
			}
			else if( inputString == "quit" )
			{
				break;
			}
		}
		
		queue.stop();
	}
	
	std::cout << "io works!" << std::endl;
	return 0;
}
