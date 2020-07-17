

#include "ProjectTree.hpp"

#include "common/assert_verify.hpp"


std::vector< boost::filesystem::path > getEGSourceCode( const boost::filesystem::path& root )
{
    std::vector< boost::filesystem::path > egSourceCode;
    
    for( boost::filesystem::directory_iterator iter( root );
        iter != boost::filesystem::directory_iterator(); ++iter )
    {
        const boost::filesystem::path& filePath = *iter;
        if( !boost::filesystem::is_directory( filePath ) )
        {
            if( boost::filesystem::extension( *iter ) == Environment::EG_FILE_EXTENSION )
            {
                if( !filePath.stem().empty() ) //ignore .eg xml files
                {
                    egSourceCode.push_back( *iter );
                }
                else
                {
                    //make this recursive...
                }
            }
        }
    }
    return egSourceCode;
}

ProjectName::ProjectName( Environment& environment, const boost::filesystem::path& root )
	:	m_path( root ),
		m_project( m_path / Environment::EG_FILE_EXTENSION, environment, 
			XMLManager::load( m_path / Environment::EG_FILE_EXTENSION)->Project() )
{
	m_sourceFiles = getEGSourceCode( m_path );
	
}
void ProjectName::print( std::ostream& os )
{
	os << "      ProjectName: " << name() << "\n";
}
	
void ProjectName::getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const
{
	for( auto& path : m_sourceFiles )
	{
		pathMap.insert( std::make_pair( m_path, path.filename() ) );
	}
}
	
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
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

void HostName::getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const
{
	for( ProjectName::Ptr pProjectName : m_projects )
	{
		pProjectName->getSourceFilesMap( pathMap );
	}
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
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

void Coordinator::getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const
{
	for( HostName::Ptr pHostName : m_hostNames )
	{
		pHostName->getSourceFilesMap( pathMap );
	}
}

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
ProjectName::Ptr recurseProjectFolder( Environment& environment, const boost::filesystem::path& p )
{
	ProjectName::Ptr pProject;
	for( auto& directoryItem : boost::filesystem::directory_iterator( p ) )
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

HostName::Ptr recurseHostFolder( Environment& environment, const boost::filesystem::path& p, const std::string& projectName )
{
	HostName::Ptr pHostName;
	
	for( auto& directoryItem : boost::filesystem::directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) && 
			directoryItem.path().filename() == projectName )
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

Coordinator::Ptr recurseCoordinatorFolder( Environment& environment, const boost::filesystem::path& p, const std::string& projectName )
{
	Coordinator::Ptr pCoordinator;
	
	for( auto& directoryItem : boost::filesystem::directory_iterator( p ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			if( HostName::Ptr pHostName = recurseHostFolder( environment, directoryItem, projectName ) )
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

ProjectTree::ProjectTree( Environment& environment, const boost::filesystem::path& root, const std::string& projectName )
	:	m_path( root ),
		m_projectName( projectName )
{
    commonCtor( environment );
}

ProjectTree::ProjectTree( Environment& environment, const boost::filesystem::path& root, 
	const std::string& coordinatorName, const std::string& hostName, const std::string& projectName )
	:	m_path( root ),
		m_coordinatorName( coordinatorName ),
		m_hostName( hostName ),
		m_projectName( projectName )
{
    commonCtor( environment );
}

void ProjectTree::commonCtor( Environment& environment )
{
    //find the src folder
	boost::filesystem::path sourceFolder = getSourceFolder();
    if( !boost::filesystem::exists( sourceFolder ) )
    {
        THROW_RTE( "Project source folder does not exist: " << sourceFolder.string() );
    }
    
	for( auto& directoryItem : boost::filesystem::directory_iterator( sourceFolder ) )
	{
		if( boost::filesystem::is_directory( directoryItem ) )
		{
			if( Coordinator::Ptr pCoordinator = recurseCoordinatorFolder( environment, directoryItem, m_projectName ) )
			{
				m_coordinators.push_back( pCoordinator );
			}
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
	
	
void ProjectTree::getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const
{
	for( auto p : m_coordinators )
	{
		p->getSourceFilesMap( pathMap );
	}
}


const Project& ProjectTree::getProject() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    
	for( Coordinator::Ptr pCoordinator : m_coordinators )
	{
        if( pCoordinator->name() == m_coordinatorName )
        {
            for( HostName::Ptr pHost : pCoordinator->getHostNames() )
            {
                if( pHost->name() == m_hostName )
                {
                    for( ProjectName::Ptr pProject : pHost->getProjectNames() )
                    {
                        if( pProject->name() == m_projectName )
                        {
                            return pProject->getProject();
                        }
                    }
                    break;
                }
            }
            break;
        }
	}
    THROW_RTE( "Failed to locate project" );
}