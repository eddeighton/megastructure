
#include "activitiesHost.hpp"

#include "schema/projectTree.hpp"

#include <boost/optional.hpp>

namespace slave
{
	
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TestHostActivity::start()
{
	using namespace megastructure;
	Message message;
	{
		Message::CHQ_Alive* pAlive = message.mutable_chq_alive();
		pAlive->set_processname( m_strProcessName );
	}
	if( !m_slave.sendHost( message, m_host.getMegaClientID() ) )
	{
		m_slave.activityComplete( shared_from_this() );
	}
}

bool TestHostActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( m_host.getMegaClientID() == uiClient )
	{
		if( message.has_hcs_alive() )
		{
			const megastructure::Message::HCS_Alive& alive = message.hcs_alive();
			if( !alive.success() )
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is not alive" << std::endl;
				m_slave.removeClient( m_host.getMegaClientID() );
			}
			else
			{
				std::cout << "Client: " << uiClient << " with name: " << m_strProcessName << " is alive" << std::endl;
				m_bSuccess = true;
			}
			m_slave.activityComplete( shared_from_this() );
			return true;
		}
	}
	return false;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void TestHostsActivity::start()
{
	const auto& hosts = m_slave.getHosts().getEnrolledHosts();
	for( auto i : hosts )
	{
		megastructure::Activity::Ptr pActivity( 
			new TestHostActivity( m_slave, i.second, i.first ) );
		m_activities.push_back( pActivity );
		m_slave.startActivity( pActivity );
	}
	if( m_activities.empty() )
	{
		m_slave.activityComplete( shared_from_this() );
	}
}

bool TestHostsActivity::activityComplete( Activity::Ptr pActivity )
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
			m_slave.activityComplete( shared_from_this() );
		}
		return true;
	}
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool HostEnrollActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	using namespace megastructure;
	
	if( message.has_hcq_enroll() )
	{
		const Message::HCQ_Enroll& enroll = message.hcq_enroll();
		if( m_slave.enroll( enroll.processname(), uiClient, enroll.unique() ) )
		{
			m_slave.sendHost( chs_enroll( true, m_slave.getWorkspace(), m_slave.getName() ), uiClient );
		}
		else 
		{
			std::cout << "HostEnrollActivity error" << std::endl;
			m_slave.sendHost( chs_enroll( false ), uiClient );
		}
		
		return true;
	}
	else if( message.has_hcq_enrolleg() )
	{
		const Message::HCQ_EnrollEG& enrolleg = message.hcq_enrolleg();
		if( m_slave.enrolleg( uiClient, enrolleg.unique() ) )
		{
			m_slave.sendHostEG( chs_enrolleg( true ), uiClient );
		}
		else 
		{
			std::cout << "HostEnrollActivity error" << std::endl;
			m_slave.sendHostEG( chs_enrolleg( false ), uiClient );
		}
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void LoadHostProgramActivity::start()
{
	using namespace megastructure;
	Message message;
	{
		Message::CHQ_Load* pLoad = message.mutable_chq_load();
		pLoad->set_hostname( m_hostName );
		pLoad->set_programname( m_programName );
	}
	if( !m_slave.sendHost( message, m_host.getMegaClientID() ) )
	{
		m_slave.activityComplete( shared_from_this() );
	}
}

bool LoadHostProgramActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
	if( m_host.getMegaClientID() == uiClient )
	{
		if( message.has_hcs_load() )
		{
			const megastructure::Message::HCS_Load& load = message.hcs_load();
			if( !load.success() )
			{
				std::cout << "Client: " << uiClient << " has NOT assumed host name: " << 
					m_hostName << " with process name: " << m_programName << std::endl;
				m_slave.removeClient( m_host.getMegaClientID() );
			}
			else
			{
				std::cout << "Client: " << uiClient << " has assumed host name: " << 
					m_hostName << " with process name: " << m_programName << std::endl;
				m_bSuccess = true;
			}
			m_slave.activityComplete( shared_from_this() );
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
void LoadHostsProgramActivity::start()
{
	//first attempt to load the program tree for the program name
	Environment& environment = m_slave.getEnvironment();
	
	std::shared_ptr< ProjectTree > pProjectTree;
	
	try
	{
		pProjectTree = 
			std::make_shared< ProjectTree >( 
				environment, m_slave.getWorkspace(), m_programName );
	}
	catch( std::exception& ex )
	{
		std::cout << "Error attempting to load project tree for: " << 
			m_programName << " : " << ex.what() << std::endl;
	}
	
	if( pProjectTree )
	{
		//determine a process name to host name map where
		//the set of enrolled hosts each have a non-unique process name
		//the program has an associated process name for each host
		//the process name to host name map uses all available process name instances
		//to map to host names in the project and attempts to reuse existing equal mappings from 
		//the previous configuration
		HostMap::ProcessNameToHostNameMap processNameToHostName;
		
		const Coordinator::PtrVector& coordinators = pProjectTree->getCoordinators();
		for( Coordinator::Ptr pCoordinator : coordinators )
		{
			//we are only interested in our slave coordinator
			if( pCoordinator->name() == m_slave.getName() )
			{
				const HostName::PtrVector& hostNames = pCoordinator->getHostNames();
				for( HostName::Ptr pHostName : hostNames )
				{
					//attempt to determine the process name for the host name
					boost::optional< std::string > optProcessName;
					{
						const ProjectName::PtrVector& projectNames = pHostName->getProjectNames();
						
						VERIFY_RTE_MSG( projectNames.size() == 1U, 
							"Host : " << pHostName->name() << 
							" has incorrect number projects for projectName: " << m_programName );
							
						for( ProjectName::Ptr pProjectName : projectNames )
						{
							if( pProjectName->name() == m_programName )
							{
								const megaxml::Host& host = pProjectName->getProject().getHost();
								VERIFY_RTE_MSG( host.Command().size() == 1U, 
									"Host has incorrect number of commands in project: " << 
									pProjectName->getProject().getProjectDir().string() );
								for( const std::string& strCommand : host.Command() )
								{
									boost::filesystem::path commandPath = 
										environment.expand( strCommand );
									optProcessName = commandPath.filename().string();
									break;
								}
							}
							break;
						}
					}
					if( optProcessName )
					{
						processNameToHostName.insert( 
							std::make_pair( optProcessName.get(), pHostName->name() ) );
					}
				}
			}
		}
		
		//given processNameToHostName can now determine the new association of hosts to hostNames
		std::cout << "Found process name to host name pairs:" << std::endl;
		for( auto& i : processNameToHostName )
		{
			std::cout << i.first << " : " << i.second << std::endl;
		}
		
		std::vector< Host > unmappedClients; 
		std::vector< std::string > unmappedHostNames;
		HostMap newMapping( m_slave.getHosts(), processNameToHostName, unmappedClients, unmappedHostNames );
		
		for( Host host : unmappedClients )
		{
			//start unload program activity
			megastructure::Activity::Ptr pActivity( 
				new LoadHostProgramActivity( m_slave, host, "", "" ) );
			m_loadActivities.insert( pActivity );
			m_slave.startActivity( pActivity );
		}
		
		for( auto& i : newMapping.getHostNameMapping() )
		{
			const std::string& strHostName = i.first;
			const Host& host = i.second;
			
			//start unload program activity
			megastructure::Activity::Ptr pActivity( 
				new LoadHostProgramActivity( m_slave, host, strHostName, m_programName ) );
			m_loadActivities.insert( pActivity );
			m_slave.startActivity( pActivity );
		}
		
		//TODO - actually use the return values to track the result of the program load
		m_slave.setHostMap( newMapping );
		
		m_slave.calculateNetworkAddressTable( pProjectTree );
		
	}
	
	if( m_loadActivities.empty() )
	{
		m_slave.activityComplete( shared_from_this() );
	}
}

bool LoadHostsProgramActivity::activityComplete( Activity::Ptr pActivity )
{
	if( std::shared_ptr< LoadHostProgramActivity > pLoadActivity = 
		std::dynamic_pointer_cast< LoadHostProgramActivity >( pActivity ) )
	{
		auto iFind = m_loadActivities.find( pActivity );
		if( iFind != m_loadActivities.end() )
		{
			if( !pLoadActivity->Successful() )
			{
				m_bSuccess = false;
			}
			
			m_loadActivities.erase( iFind );
			if( m_loadActivities.empty() )
			{
				m_slave.activityComplete( shared_from_this() );
			}
			return true;
		}
	}
	
	return false;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
bool HostBufferActivity::clientMessage( std::uint32_t uiClient, const megastructure::Message& message )
{
    if( message.has_hcq_buffer() )
    {
        const megastructure::Message::HCQ_Buffer& bufferRequest = message.hcq_buffer();
        
        //std::cout << "Received request for shared buffer: " << bufferRequest.buffername() << 
        //    " size: " << bufferRequest.size() << std::endl;
        
        const std::string strSharedBufferName =
            m_slave.getSharedBufferName( bufferRequest.buffername(), bufferRequest.size() );
        
		m_slave.sendHost( megastructure::chs_buffer( bufferRequest.buffername(), strSharedBufferName ), uiClient );

        return true;
    }
    
    return false;
}



}