

#include "ProjectTree.hpp"

#include "common/assert_verify.hpp"

ProjectName::ProjectName( Environment& environment, const boost::filesystem::path& root )
	:	m_path( root ),
		m_project( m_path / Environment::EG_FILE_EXTENSION, environment, XMLManager::load( m_path )->Project() )
{
}
void ProjectName::print( std::ostream& os )
{
	os << "      ProjectName: " << name() << "\n";
}
	
HostName::HostName( const boost::filesystem::path& root )
	:	m_path( root )
{
}

void HostName::print( std::ostream& os )
{
	os << "    HostName: " << name() << "\n";
	for( ProjectName::Ptr pProjectName : m_projects )
	{
		pProjectName->print( os );
	}
}

Coordinator::Coordinator( const boost::filesystem::path& root )
	:	m_path( root )
{
	
}

void Coordinator::print( std::ostream& os )
{
	os << "  Coordinator: " << name() << "\n";
	for( HostName::Ptr pHostName : m_hostNames )
	{
		pHostName->print( os );
	}
}
/*
void recurseModuleFolder( const boost::filesystem::path& p )
{
	for( auto& directoryItem : boost::filesystem::recursive_directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			recurseModuleFolder( directoryItem );
		}
		else
		{
			
		}
	}
}
*/

ProjectName::Ptr recurseProjectFolder( Environment& environment, const boost::filesystem::path& p )
{
	ProjectName::Ptr pProject;
	for( auto& directoryItem : boost::filesystem::recursive_directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			//recurseModuleFolder( directoryItem );
		}
		else
		{
			if( directoryItem.path().filename().string() == Environment::EG_FILE_EXTENSION )
			{
				VERIFY_RTE_MSG( !pProject, "Duplicate .eg files found in project folder: " << p );
				//found project
				pProject = std::make_shared< ProjectName >( environment, p );
			}
		}
	}
	
	return pProject;
}

HostName::Ptr recurseHostFolder( Environment& environment, const boost::filesystem::path& p )
{
	HostName::Ptr pHostName;
	
	for( auto& directoryItem : boost::filesystem::recursive_directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			if( ProjectName::Ptr pProject = recurseProjectFolder( environment, directoryItem ) )
			{
				if( !pHostName )
				{
					pHostName = std::make_shared< HostName >( p );
				}
				pHostName->addProjectName( pProject );
			}
		}
	}
	
	return pHostName;
}

Coordinator::Ptr recurseCoordinatorFolder( Environment& environment, const boost::filesystem::path& p )
{
	Coordinator::Ptr pCoordinator;
	
	for( auto& directoryItem : boost::filesystem::recursive_directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			if( HostName::Ptr pHostName = recurseHostFolder( environment, directoryItem ) )
			{
				if( !pCoordinator )
				{
					pCoordinator = std::make_shared< Coordinator >( p );
				}
				pCoordinator->addHostName( pHostName );
			}
		}
		else
		{
			
		}
	}
	return pCoordinator;
}

ProjectTree::ProjectTree( Environment& environment, const boost::filesystem::path& root )
	:	m_path( root )
{
	for( auto& directoryItem : boost::filesystem::recursive_directory_iterator( m_path ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			if( Coordinator::Ptr pCoordinator = recurseCoordinatorFolder( environment, directoryItem ) )
			{
				m_coordinators.push_back( pCoordinator );
			}
		}
		else
		{
			
		}
	}
}

void ProjectTree::print( std::ostream& os )
{
	os << "Project: " << m_path.string() << "\n";
	for( Coordinator::Ptr pCoordinator : m_coordinators )
	{
		pCoordinator->print( os );
	}
}
	