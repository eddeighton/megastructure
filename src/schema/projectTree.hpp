
#ifndef PROJECT_TREE_17_APRIL_2020
#define PROJECT_TREE_17_APRIL_2020

#include "project.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <map>

class ProjectName
{
public:
	using Ptr = std::shared_ptr< ProjectName >;
	using PtrVector = std::vector< Ptr >;

	ProjectName( Environment& environment, const boost::filesystem::path& root );
	
	std::string name() const { return m_path.filename().string(); }
	const std::vector< boost::filesystem::path >& sourceFiles() const { return m_sourceFiles; }
	
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
	
	void print( std::ostream& os );
	
	
	const boost::filesystem::path& getRootPath() const { return m_path; }
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
private:
	boost::filesystem::path m_path;
	std::string m_projectName;
	Coordinator::PtrVector m_coordinators;
};


#endif //PROJECT_TREE_17_APRIL_2020