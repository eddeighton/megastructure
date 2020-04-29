
#include "slave.hpp"

#include "activitiesMaster.hpp"
#include "activitiesHost.hpp"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include "boost/filesystem.hpp"

namespace slave
{
	
	void getWorkspaceAndSlaveNameFromPath( const boost::filesystem::path& thePath, 
		boost::filesystem::path& workspace, std::string& strSlaveName )
	{
		const boost::filesystem::path canonPath = 
			boost::filesystem::edsCannonicalise(
				boost::filesystem::absolute( thePath ) );
		
		VERIFY_RTE_MSG( boost::filesystem::exists( canonPath ), 
			"Specified slave path does not exist: " << thePath.string() );
		
		workspace = canonPath.parent_path();
		strSlaveName = canonPath.stem().string();
	}

	Slave::Slave( 	Environment& environment,
					const std::string& strMasterIP, 
					const std::string strMasterPort, 
					const std::string& strSlavePort, 
					const boost::filesystem::path& strSlavePath )
		:	m_environment( environment ),
			m_server( strSlavePort ),
			m_client( strMasterIP, strMasterPort )
	{
		getWorkspaceAndSlaveNameFromPath( strSlavePath, m_workspacePath, m_strSlaveName );
		
		megastructure::Queue& queue = m_queue;
		megastructure::Server& server = m_server;
		megastructure::Client& client = m_client;
	
		m_zeromqClient = std::thread(
			[ &client, &queue ]()
			{
				megastructure::readClient( client, queue );
			});
			
		m_zeromqServer = std::thread(
			[ &server, &queue ]()
			{
				megastructure::readServer( server, queue );
			});
		
		m_queue.startActivity( new AliveTestActivity( *this ) );
		m_queue.startActivity( new MasterEnrollActivity( *this ) );
		m_queue.startActivity( new HostEnrollActivity( *this ) );
		m_queue.startActivity( new LoadProgramActivity( *this ) );
	}
	
	Slave::~Slave()
	{
		m_queue.stop();
		m_server.stop();
		m_client.stop();
		m_zeromqClient.join();
		m_zeromqServer.join();
	}
	
	
}