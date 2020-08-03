
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
    
    std::string getComponentFileName( bool bDebug ) const;
    std::string getComponentFileNameExt( bool bDebug ) const;
    
	const Coordinator::PtrVector& getCoordinators() const { return m_coordinators; }
    const Project& getProject() const;
	
	const boost::filesystem::path& getRootPath() const { return m_path; }
	
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
    std::vector< boost::filesystem::path > getSystemIncludes() const;
    std::vector< boost::filesystem::path > getUserIncludes( const Environment& environment ) const;
	
	boost::filesystem::path getSourceFolder() const;
	boost::filesystem::path getInterfaceFolder() const;
	boost::filesystem::path getImplFolder() const;
	boost::filesystem::path getBuildFolder() const;
	boost::filesystem::path getParserDatabaseFile() const;
	boost::filesystem::path getInterfaceDatabaseFile() const;
	boost::filesystem::path getInterfacePCH() const;
    boost::filesystem::path getInterfaceHeader() const;
    boost::filesystem::path getIncludeHeader() const;
    boost::filesystem::path getIncludePCH() const;
	
    boost::filesystem::path getTUDBName( const std::string& strTUName ) const;
    
	boost::filesystem::path getCoroutineFrameSourceFilePath( const Environment& environment ) const;
	boost::filesystem::path getBasicSchedulerFilePath( const Environment& environment ) const;
		
	static void collateIncludeDirectories( 
		const Environment& environment,
		std::set< boost::filesystem::path >& uniquified, 
		std::vector< boost::filesystem::path >& directories,
		const std::string& strDirectory );

	std::vector< boost::filesystem::path > getIncludeDirectories( const Environment& environment ) const;
	std::vector< boost::filesystem::path > getImplIncludeDirectories( const Environment& environment ) const;
	boost::filesystem::path getOperationsHeader( const std::string& strTUName ) const;
	boost::filesystem::path getOperationsPCH( const std::string& strTUName ) const;
	boost::filesystem::path getImplementationSource( const std::string& strTUName ) const;
    std::string getStructuresInclude() const;
	boost::filesystem::path getDataStructureSource() const;
	boost::filesystem::path getAnalysisFileName() const;
    
    ///////////////////////////////////////////////
    //VERIFY_RTE( m_coordinatorName && m_hostName ); 
    
	std::string getNetStateSourceInclude() const;
	boost::filesystem::path getNetStateSource() const;
	boost::filesystem::path getRuntimeSource() const;
	boost::filesystem::path getPythonSource() const;
	std::string getUnrealInterfaceInclude() const;
	boost::filesystem::path getUnrealInterface() const;
	boost::filesystem::path getUnrealSource() const;
	boost::filesystem::path getObjectName( const std::string& strTUName, const boost::filesystem::path& binPath ) const;
	boost::filesystem::path getObjectFile( const boost::filesystem::path& sourceFile, const boost::filesystem::path& binPath ) const;
	boost::filesystem::path getEGComponentSource() const;
        
private:
	boost::filesystem::path m_path;
	boost::optional< std::string > m_coordinatorName;
	boost::optional< std::string > m_hostName;
	std::string m_projectName;
	Coordinator::PtrVector m_coordinators;
};


#endif //PROJECT_TREE_17_APRIL_2020