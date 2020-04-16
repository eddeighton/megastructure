
#include "megastructure/clientServer.hpp"
#include "megastructure/queue.hpp"
#include "megastructure/clientMap.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

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

class MasterEnrollActivity : public megastructure::Activity
{
public:
	MasterEnrollActivity( 
				megastructure::Queue& queue, 
				megastructure::Client& client,
				const std::string& name ) 
		:	m_queue( queue ),
			m_client( client ),
			m_name( name )
	{
		
	}
	
	virtual void start()
	{
		std::cout << "MasterEnrollActivity started" << std::endl;
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
			std::cout << "MasterEnrollActivity Got response: " << 
				message.mss_enroll().success() << std::endl;
			m_queue.activityComplete( shared_from_this() );
			return true;
		}
		return false;
	}
	
private:
	megastructure::Queue& m_queue;
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
		:	m_queue( queue ),
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
	megastructure::Queue& m_queue;
	megastructure::Client& m_client;
	std::string m_name;
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

class TestHostActivity : public megastructure::Activity
{
public:
	TestHostActivity( megastructure::ClientMap& clients,
						megastructure::Queue& queue, 
						megastructure::Server& server, 
						std::uint32_t clientID, 
						const std::string& strProcessName ) 
		:	m_queue( queue ),
			m_clients(clients),
			m_server( server ),
			m_clientID( clientID ),
			m_strProcessName( strProcessName ),
			m_bSuccess( false )
	{
		
	}
	
	virtual void start()
	{
		using namespace megastructure;
		Message message;
		{
			Message::CHQ_Alive* pAlive = message.mutable_chq_alive();
			pAlive->set_processname( m_strProcessName );
		}
		if( !m_server.send( message, m_clientID ) )
		{
			m_clients.removeClient( m_clientID );
			m_queue.activityComplete( shared_from_this() );
		}
	}
	
	virtual bool clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
	{
		if( m_clientID == uiClient )
		{
			if( message.has_hcs_alive() )
			{
				const megastructure::Message::HCS_Alive& alive = message.hcs_alive();
				if( !alive.success() )
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is not alive" << std::endl;
					m_clients.removeClient( m_clientID );
				}
				else
				{
					std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is alive" << std::endl;
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
		return m_strProcessName;
	}
	
	std::uint32_t getClientID() const 
	{
		return m_clientID;
	}
private:
	megastructure::Queue& m_queue;
	megastructure::ClientMap& m_clients;
	megastructure::Server& m_server;
	std::uint32_t m_clientID;
	std::string m_strProcessName;
	bool m_bSuccess;
};

class TestHostsActivity : public megastructure::Activity
{
public:
	TestHostsActivity( megastructure::ClientMap& clients,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
			m_clients( clients ),
			m_server( server )
	{
		
	}
	
	virtual void start()
	{
		const megastructure::ClientMap::ClientIDMap& clients = m_clients.getClients();
		for( megastructure::ClientMap::ClientIDMap::const_iterator 
			i = clients.begin(), iEnd = clients.end();
			i!=iEnd; ++i )
		{
			megastructure::Activity::Ptr pActivity( 
				new TestHostActivity( m_clients, m_queue, m_server, i->second, i->first ) );
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
	megastructure::ClientMap& m_clients;
	megastructure::Server& m_server;
	megastructure::Activity::PtrList m_activities;
};


class ListHostsActivity : public megastructure::Activity
{
public:
	ListHostsActivity( megastructure::ClientMap& clients,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
			m_clients( clients ),
			m_server( server )
	{
		
	}
	
	virtual void start()
	{
		const megastructure::ClientMap::ClientIDMap& clients = m_clients.getClients();
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
	megastructure::ClientMap& m_clients;
	megastructure::Server& m_server;
};

class HostEnrollActivity : public megastructure::Activity
{
public:
	HostEnrollActivity( megastructure::ClientMap& clients,
				megastructure::Queue& queue, 
				megastructure::Server& server ) 
		:	m_queue( queue ),
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
				if( !m_server.send( chs_enroll( true, "/someplace", "AProgramFolderName" ), uiClient ) )
				{
					m_clients.removeClient( uiClient );
				}
			}
			else 
			{
				std::uint32_t uiExisting;
				if( m_clients.getClientID( enroll.processname(), uiExisting ) )
				{
					std::cout << "Enroll attempting for: " << enroll.processname() << " which has existing client of: " << uiExisting << std::endl;
					std::shared_ptr< TestHostActivity > pTest = 
						std::make_shared< TestHostActivity >( m_clients, m_queue, m_server, uiExisting, enroll.processname() );
					m_testsMap.insert( std::make_pair( pTest, uiClient ) );
					m_queue.startActivity( pTest );
					return true;
				}
				else
				{
					std::cout << "Enroll denied for: " << enroll.processname() << " for client: " << uiClient << std::endl;
					if( !m_server.send( chs_enroll( false ), uiClient ) )
					{
						m_clients.removeClient( uiClient );
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
		if( std::shared_ptr< TestHostActivity > pTest = 
				std::dynamic_pointer_cast< TestHostActivity >( pActivity ) )
		{
			std::map< std::shared_ptr< TestHostActivity >, std::uint32_t >::iterator 
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
					if( !m_server.send( chs_enroll( false ), clientID ) )
					{
						m_clients.removeClient( clientID );
					}
				}
				else
				{
					std::cout << "Existing client: " << testedID << " is not alive so allowing enrollment of new client: " << 
						clientID << " as: " << pTest->getName() << std::endl;
					//testing the existing client indicated it was actually dead so can enroll the new one
					if( m_clients.enroll( pTest->getName(), clientID ) )
					{
						if( !m_server.send( chs_enroll( true, "/someplace", "AProgramFolderName" ), clientID ) )
						{
							m_clients.removeClient( clientID );
						}
					}
					else
					{
						std::cout << "Enroll denied after retry for: " << pTest->getName() << " for client: " << clientID << std::endl;
						if( !m_server.send( chs_enroll( false ), clientID ) )
						{
							m_clients.removeClient( clientID );
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
	megastructure::ClientMap& m_clients;
	megastructure::Server& m_server;
	std::map< std::shared_ptr< TestHostActivity >, std::uint32_t > m_testsMap;
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
		
		queue.startActivity( new AliveTestActivity( queue, client, args.slave_name ) );
		queue.startActivity( new MasterEnrollActivity( queue, client, args.slave_name ) );
		queue.startActivity( new HostEnrollActivity( clients, queue, server ) );
		
		std::string str, strResponse;
		while( true )
		{
			std::cin >> str;
			
			if( str == "test" )
				queue.startActivity( new TestHostsActivity( clients, queue, server ) );
			else if( str == "list" )
				queue.startActivity( new ListHostsActivity( clients, queue, server ) );
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
