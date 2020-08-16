

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


std::string ProjectTree::getComponentFileName( bool bDebug ) const
{
    std::ostringstream os;
    if( bDebug )
        os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << "d";
    else
        os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName();
    return os.str();
}
std::string ProjectTree::getComponentFileNameExt( bool bDebug ) const
{
    std::ostringstream os;
    if( bDebug )
        os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << "d.dll";
    else
        os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << ".dll";
    return os.str();
}

std::vector< boost::filesystem::path > ProjectTree::getSystemIncludes() const
{
    std::vector< boost::filesystem::path > includes;
    
    return includes;
}
std::vector< boost::filesystem::path > ProjectTree::getUserIncludes( const Environment& environment ) const
{
    std::vector< boost::filesystem::path > includes;
    
    includes.push_back( "boost/filesystem.hpp" );
    
    includes.push_back( environment.expand( "egcomponent/traits.hpp" ) );
    
    includes.push_back( "eg/include.hpp" );
    includes.push_back( "eg_runtime/eg_runtime.hpp" );

    return includes;
}

boost::filesystem::path ProjectTree::getSourceFolder() const
{
    return m_path / "src";
}

boost::filesystem::path ProjectTree::getInterfaceFolder() const
{
    return m_path / "interface" / m_projectName;
}

boost::filesystem::path ProjectTree::getImplFolder() const
{
    return m_path / "impl";
}

boost::filesystem::path ProjectTree::getBuildFolder() const
{
    return m_path / "build" / m_projectName;
}

boost::filesystem::path ProjectTree::getParserDatabaseFile() const
{
    return getInterfaceFolder() / "parser.db";
}
boost::filesystem::path ProjectTree::getInterfaceDatabaseFile() const
{
    return getInterfaceFolder() / "interface.db";
}
boost::filesystem::path ProjectTree::getInterfacePCH() const
{
    return getInterfaceFolder() / "interface.pch";
}
boost::filesystem::path ProjectTree::getInterfaceHeader() const
{
    return getInterfaceFolder() / "interface.hpp";
}

boost::filesystem::path ProjectTree::getIncludeHeader() const
{
    return getInterfaceFolder() / "include.hpp";
}

boost::filesystem::path ProjectTree::getIncludePCH() const
{
    return getInterfaceFolder() / "include.pch";
}

boost::filesystem::path ProjectTree::getCoroutineFrameSourceFilePath( const Environment& environment ) const
{
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            environment.expand( "${EG}/include/eg/frame.cpp" ) ) );
}
boost::filesystem::path ProjectTree::getBasicSchedulerFilePath( const Environment& environment ) const
{
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            environment.expand( "${EG}/include/eg/basic_scheduler.cpp" ) ) );
}

void ProjectTree::collateIncludeDirectories( 
    const Environment& environment,
    std::set< boost::filesystem::path >& uniquified, 
    std::vector< boost::filesystem::path >& directories,
    const std::string& strDirectory )
{
    const boost::filesystem::path absPath =
        boost::filesystem::edsCannonicalise(
            boost::filesystem::absolute( 
                environment.expand( strDirectory ) ) );
            
    if( 0 == uniquified.count( absPath ) )
    {
        uniquified.insert( absPath );
        directories.push_back( absPath );
    }
}

std::vector< boost::filesystem::path > ProjectTree::getIncludeDirectories( const Environment& environment ) const
{
    std::set< boost::filesystem::path > uniquified;
    std::vector< boost::filesystem::path > directories;
    
    collateIncludeDirectories( environment, uniquified, directories, "${BOOST}/include/boost-1_73" );
    collateIncludeDirectories( environment, uniquified, directories, "${PYBIND}/include" );
    collateIncludeDirectories( environment, uniquified, directories, "${PYTHONHOME}/include" );
    collateIncludeDirectories( environment, uniquified, directories, "${EG}/include" );
    collateIncludeDirectories( environment, uniquified, directories, "${MEGA}/include" );
    collateIncludeDirectories( environment, uniquified, directories, "${PROTOBUF}/include" );
    collateIncludeDirectories( environment, uniquified, directories, "${MESSAGEPACK}/include" );
    
    directories.push_back( 
        boost::filesystem::edsCannonicalise(
            boost::filesystem::absolute( 
                getInterfaceFolder() ) ) );
    
    /*if( m_host.Directories_present() )
        collateIncludeDirectories( m_environment, uniquified, directories, m_host.Directories() );
    
    for( const megaxml::Package& package : m_packages )
    {
        if( package.Directories_present() )
            collateIncludeDirectories( m_environment, uniquified, directories, package.Directories() );
    }*/
    
    return directories;
}

std::vector< boost::filesystem::path > ProjectTree::getImplIncludeDirectories( const Environment& environment ) const
{
    std::set< boost::filesystem::path > uniquified;
    std::vector< boost::filesystem::path > directories;
    
    //collateIncludeDirectories( environment, uniquified, directories, "${BOOST}/include/boost-1_73" );
    //collateIncludeDirectories( environment, uniquified, directories, "${PYBIND}/include" );
    //collateIncludeDirectories( environment, uniquified, directories, "${PYTHONHOME}/include" );
    //collateIncludeDirectories( environment, uniquified, directories, "${EG}/include" );
    //collateIncludeDirectories( environment, uniquified, directories, "${MEGA}/include" );
    //collateIncludeDirectories( environment, uniquified, directories, "${PROTOBUF}/include" );
    //collateIncludeDirectories( environment, uniquified, directories, "${MESSAGEPACK}/include" );
    
    directories.push_back( 
        boost::filesystem::edsCannonicalise(
            boost::filesystem::absolute( 
                getInterfaceFolder() ) ) );
    
    /*if( m_host.Directories_present() )
        collateIncludeDirectories( m_environment, uniquified, directories, m_host.Directories() );
    
    for( const megaxml::Package& package : m_packages )
    {
        if( package.Directories_present() )
            collateIncludeDirectories( m_environment, uniquified, directories, package.Directories() );
    }*/
    
    return directories;
}

boost::filesystem::path ProjectTree::getTUDBName( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << "tu_" << strTUName << ".db";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getOperationsIncludeHeader( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << strTUName << "_include.hpp";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getOperationsIncludePCH( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << strTUName << "_include.pch";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getOperationsHeader( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << strTUName << "_operations.hpp";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getOperationsPublicPCH( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << strTUName << "_public.pch";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getOperationsPrivatePCH( const std::string& strTUName ) const
{
    std::ostringstream os;
    os << strTUName << "_private.pch";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

boost::filesystem::path ProjectTree::getImplementationSource( const std::string& strTUName ) const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    std::ostringstream os;
    os << strTUName << "_operations.cpp";
    
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / os.str() ) );
}

std::string ProjectTree::getStructuresInclude() const
{
    std::ostringstream os;
    os << "structures.hpp";
    return os.str();
}

boost::filesystem::path ProjectTree::getDataStructureSource() const
{
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / getStructuresInclude() ) );
}

boost::filesystem::path ProjectTree::getAnalysisFileName() const
{
    std::ostringstream os;
    os << "database.db";
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getInterfaceFolder() / os.str() ) );
}

std::string ProjectTree::getNetStateSourceInclude() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    std::ostringstream os;
    os << "netstate.hpp";
    return os.str();
}

boost::filesystem::path ProjectTree::getNetStateSource() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / getNetStateSourceInclude() ) );
            
}

boost::filesystem::path ProjectTree::getRuntimeSource() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "runtime.cpp" ) );
}

boost::filesystem::path ProjectTree::getPythonSource() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "python.cpp" ) );
}

std::string ProjectTree::getUnrealInterfaceInclude() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    std::ostringstream os;
    os << "unreal.hpp";
    return os.str();
}
boost::filesystem::path ProjectTree::getUnrealInterface() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / getUnrealInterfaceInclude() ) );
}

boost::filesystem::path ProjectTree::getUnrealSource() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "unreal.cpp" ) );
}

boost::filesystem::path ProjectTree::getObjectName( const std::string& strTUName, const boost::filesystem::path& binPath ) const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    std::ostringstream os;
    os << strTUName << "_object.obj";
    return boost::filesystem::edsCannonicalise(
                boost::filesystem::absolute( 
                    binPath / os.str() ) );
}

boost::filesystem::path ProjectTree::getObjectFile( const boost::filesystem::path& sourceFile, const boost::filesystem::path& binPath ) const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    std::ostringstream os;
    os << sourceFile.stem().string() << "_object.obj";
    return boost::filesystem::edsCannonicalise(
                boost::filesystem::absolute( 
                    binPath / os.str() ) );
}


boost::filesystem::path ProjectTree::getEGComponentSource() const
{
    VERIFY_RTE( m_coordinatorName && m_hostName );
    return boost::filesystem::edsCannonicalise(
        boost::filesystem::absolute( 
            getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "component.cpp" ) );
}