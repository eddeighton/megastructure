
#ifndef PROJECT_TREE_17_APRIL_2020
#define PROJECT_TREE_17_APRIL_2020

#include "project.hpp"

#include "common/file.hpp"

#include <boost/filesystem.hpp>

#include <string>
#include <memory>
#include <vector>
#include <ostream>
#include <map>
#include <set>

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
	
    std::vector< boost::filesystem::path > getSystemIncludes() const
	{
		std::vector< boost::filesystem::path > includes;
		
		return includes;
	}
    std::vector< boost::filesystem::path > getUserIncludes() const
	{
		std::vector< boost::filesystem::path > includes;
		
		includes.push_back( "eg/include.hpp" );
		
		return includes;
	}
	
	boost::filesystem::path getInterfaceFolder() const
	{
		return m_path / "interface" / m_projectName;
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
		
	std::string getCompilerFlags() const
	{
		return "";
	}
	
    boost::filesystem::path getIncludeHeader() const
	{
		return getInterfaceFolder() / "include.hpp";
	}
	
    boost::filesystem::path getIncludePCH() const
	{
		return getInterfaceFolder() / "include.pch";
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
		
		collateIncludeDirectories( environment, uniquified, directories, "${EG}/include" );
		collateIncludeDirectories( environment, uniquified, directories, "${BOOST}/include/boost-1_73" );
		
		/*if( m_host.Directories_present() )
			collateIncludeDirectories( m_environment, uniquified, directories, m_host.Directories() );
		
		for( const megaxml::Package& package : m_packages )
		{
			if( package.Directories_present() )
				collateIncludeDirectories( m_environment, uniquified, directories, package.Directories() );
		}*/
		
		return directories;
	}

    std::size_t getFiberStackSize() const
	{
		return 4096;
	}
private:
	boost::filesystem::path m_path;
	std::string m_projectName;
	Coordinator::PtrVector m_coordinators;
};


#endif //PROJECT_TREE_17_APRIL_2020