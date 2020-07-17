
#ifndef PROJECT_TREE_17_APRIL_2020
#define PROJECT_TREE_17_APRIL_2020

#include "project.hpp"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include "boost/filesystem.hpp"
#include "boost/optional.hpp"

#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <map>
#include <set>
#include <sstream>

class ProjectName
{
public:
	using Ptr = std::shared_ptr< ProjectName >;
	using PtrVector = std::vector< Ptr >;

	ProjectName( Environment& environment, const boost::filesystem::path& root );
	
	std::string name() const { return m_path.filename().string(); }
	const std::vector< boost::filesystem::path >& sourceFiles() const { return m_sourceFiles; }
	const Project& getProject() const { return m_project; }
	
	void print( std::ostream& os );
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
private:
	boost::filesystem::path m_path;
	std::vector< boost::filesystem::path > m_sourceFiles;
	Project m_project;
};

class HostName
{
public:
	using Ptr = std::shared_ptr< HostName >;
	using PtrVector = std::vector< Ptr >;

	HostName( const boost::filesystem::path& root );
	
	std::string name() const { return m_path.filename().string(); }
	const ProjectName::PtrVector& getProjectNames() const { return m_projects; }
	
	void addProjectName( ProjectName::Ptr pProjectName )
	{
		m_projects.push_back( pProjectName );
	}
	
	void print( std::ostream& os );
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
private:
	boost::filesystem::path m_path;
	ProjectName::PtrVector m_projects;
};

class Coordinator
{
public:
	using Ptr = std::shared_ptr< Coordinator >;
	using PtrVector = std::vector< Ptr >;

	Coordinator( const boost::filesystem::path& root );
	
	std::string name() const { return m_path.filename().string(); }
	const HostName::PtrVector& getHostNames() const { return m_hostNames; }
	
	void addHostName( HostName::Ptr pHostName )
	{
		m_hostNames.push_back( pHostName );
	}
	
	void print( std::ostream& os );
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
private:
	boost::filesystem::path m_path;
	HostName::PtrVector m_hostNames;
};


class ProjectTree
{
public:
	ProjectTree( Environment& environment, const boost::filesystem::path& root, const std::string& projectName );
	ProjectTree( Environment& environment, const boost::filesystem::path& root, 
		const std::string& coordinatorName, const std::string& hostName, const std::string& projectName );
private:
    void commonCtor( Environment& environment );
public:
	
	void print( std::ostream& os );
	
	const std::string& getCoordinatorName() const { return m_coordinatorName.get(); }
	const std::string& getHostName() const { return m_hostName.get(); }
	const std::string& getProjectName() const { return m_projectName; }
    
    std::string getComponentFileName( bool bDebug ) const
    {
        std::ostringstream os;
        if( bDebug )
            os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << "d";
        else
            os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName();
        return os.str();
    }
    std::string getComponentFileNameExt( bool bDebug ) const
    {
        std::ostringstream os;
        if( bDebug )
            os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << "d.dll";
        else
            os << getCoordinatorName() << '_' << getHostName() << '_' << getProjectName() << ".dll";
        return os.str();
    }
    
	const Coordinator::PtrVector& getCoordinators() const { return m_coordinators; }
    const Project& getProject() const;
	
	const boost::filesystem::path& getRootPath() const { return m_path; }
	
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
    std::vector< boost::filesystem::path > getSystemIncludes() const
	{
		std::vector< boost::filesystem::path > includes;
		
		return includes;
	}
    std::vector< boost::filesystem::path > getUserIncludes( const Environment& environment ) const
	{
		std::vector< boost::filesystem::path > includes;
		
		includes.push_back( "boost/filesystem.hpp" );
		
		includes.push_back( "pybind11/pybind11.h" );
		includes.push_back( "pybind11/stl.h" );
		includes.push_back( "pybind11/stl_bind.h" );
		includes.push_back( environment.expand( "${PYBIND}/pybind11_helpers.hpp" ) );
		
		includes.push_back( environment.expand( "egcomponent/traits.hpp" ) );
		
		includes.push_back( "eg/include.hpp" );
		includes.push_back( "eg_runtime/eg_runtime.hpp" );

		return includes;
	}
	
	boost::filesystem::path getSourceFolder() const
	{
		return m_path / "src";
	}
    
	boost::filesystem::path getInterfaceFolder() const
	{
		return m_path / "interface" / m_projectName;
	}
	
	boost::filesystem::path getImplFolder() const
	{
		return m_path / "impl";
	}
	
	boost::filesystem::path getBuildFolder() const
	{
		return m_path / "build" / m_projectName;
	}
	
	boost::filesystem::path getParserDatabaseFile() const
	{
		return getInterfaceFolder() / "parser.db";
	}
	boost::filesystem::path getInterfaceDatabaseFile() const
	{
		return getInterfaceFolder() / "interface.db";
	}
	boost::filesystem::path getInterfacePCH() const
	{
		return getInterfaceFolder() / "interface.pch";
	}
    boost::filesystem::path getInterfaceHeader() const
	{
		return getInterfaceFolder() / "interface.hpp";
	}
	
    boost::filesystem::path getIncludeHeader() const
	{
		return getInterfaceFolder() / "include.hpp";
	}
	
    boost::filesystem::path getIncludePCH() const
	{
		return getInterfaceFolder() / "include.pch";
	}
	
	
    boost::filesystem::path getTUDBName( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << "tu_" << strTUName << ".db";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
		
	static void collateIncludeDirectories( 
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

	std::vector< boost::filesystem::path > getIncludeDirectories( const Environment& environment ) const
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

	std::vector< boost::filesystem::path > getImplIncludeDirectories( const Environment& environment ) const
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
	
	boost::filesystem::path getOperationsHeader( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << strTUName << "_operations.hpp";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getOperationsPCH( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << strTUName << "_operations.pch";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getImplementationSource( const std::string& strTUName ) const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		std::ostringstream os;
		os << strTUName << "_operations.cpp";
        
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / os.str() ) );
	}
    
    std::string getStructuresInclude() const
    {
		std::ostringstream os;
		os << "structures.hpp";
        return os.str();
    }

	boost::filesystem::path getDataStructureSource() const
	{
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / getStructuresInclude() ) );
	}
	
	boost::filesystem::path getAnalysisFileName() const
	{
		std::ostringstream os;
		os << "database.db";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
    
    
    ///////////////////////////////////////////////
    //VERIFY_RTE( m_coordinatorName && m_hostName ); 
    
	std::string getNetStateSourceInclude() const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		std::ostringstream os;
		os << "netstate.hpp";
        return os.str();
	}
    
	boost::filesystem::path getNetStateSource() const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / getNetStateSourceInclude() ) );
                
	}
	
	boost::filesystem::path getRuntimeSource() const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "runtime.cpp" ) );
	}
	
	boost::filesystem::path getPythonSource() const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "python.cpp" ) );
	}

	boost::filesystem::path getObjectName( const std::string& strTUName, const boost::filesystem::path& binPath ) const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		std::ostringstream os;
		os << strTUName << "_object.obj";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						binPath / os.str() ) );
	}
	
	boost::filesystem::path getObjectFile( const boost::filesystem::path& sourceFile, const boost::filesystem::path& binPath ) const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		std::ostringstream os;
		os << sourceFile.stem().string() << "_object.obj";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						binPath / os.str() ) );
	}
	

	boost::filesystem::path getEGComponentSource() const
	{
		VERIFY_RTE( m_coordinatorName && m_hostName );
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getImplFolder() / m_coordinatorName.get() / m_hostName.get() / m_projectName / "component.cpp" ) );
	}
        
private:
	boost::filesystem::path m_path;
	boost::optional< std::string > m_coordinatorName;
	boost::optional< std::string > m_hostName;
	std::string m_projectName;
	Coordinator::PtrVector m_coordinators;
};


#endif //PROJECT_TREE_17_APRIL_2020