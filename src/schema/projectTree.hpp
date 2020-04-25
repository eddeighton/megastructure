
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
#include <sstream>

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
	
	const std::string& getProjectName() const { return m_projectName; }
	
	const boost::filesystem::path& getRootPath() const { return m_path; }
	
	void getSourceFilesMap( std::multimap< boost::filesystem::path, boost::filesystem::path >& pathMap ) const;
	
    std::vector< boost::filesystem::path > getSystemIncludes() const
	{
		std::vector< boost::filesystem::path > includes;
		
		//includes.push_back( "chrono" );
		
		return includes;
	}
    std::vector< boost::filesystem::path > getUserIncludes( const Environment& environment ) const
	{
		std::vector< boost::filesystem::path > includes;
		
		includes.push_back( "boost/filesystem.hpp" );
		
		includes.push_back( "pybind11/pybind11.h" );
		includes.push_back( "pybind11/stl.h" );
		includes.push_back( "pybind11/stl_bind.h" );
		includes.push_back( "pybind11/embed.h" );
		includes.push_back( environment.expand( "${PYBIND}/pybind11_helpers.hpp" ) );
		
		
		includes.push_back( "eg/include.hpp" );
		includes.push_back( "eg_runtime/eg_runtime.hpp" );
		
//#include "W:/root/thirdparty/pybind11/install/include/pybind11/pybind11.h"
//#include "W:/root/thirdparty/pybind11/install/include/pybind11/stl.h"
//#include "W:/root/thirdparty/pybind11/install/include/pybind11/stl_bind.h"
//#include "W:/root/thirdparty/pybind11/install/include/pybind11/embed.h"
//#include "W:/root/thirdparty/pybind11/install/pybind11_helpers.hpp"

		return includes;
	}
	
	boost::filesystem::path getInterfaceFolder() const
	{
		return m_path / "interface" / m_projectName;
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
		
	std::string getCompilerFlags() const
	{
		static const std::string strFlags = //"";
		//"-D_MT -D_DLL -DNOMINMAX -DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -DBOOST_USE_WINDOWS_H -Xclang -std=c++17  -fexceptions";
		"-DWIN32_LEAN_AND_MEAN -D_MT -D_DLL -DNOMINMAX -DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -DBOOST_USE_WINDOWS_H -fexceptions -Xclang -std=c++17 -Xclang -flto-visibility-public-std -Wno-deprecated -Wno-inconsistent-missing-override";
      //<LinkerFlags>-nostdlib -lmsvcrt -Xlinker /SUBSYSTEM:CONSOLE</LinkerFlags>
		return strFlags;
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
	
	
	
	boost::filesystem::path getOperationsHeader( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << "operations_" << strTUName << ".hpp";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getOperationsPCH( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << "operations_" << strTUName << ".pch";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getImplementationSource( const std::string& strTUName ) const
	{
		std::ostringstream os;
		os << "operations_" << strTUName << ".cpp";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}

	boost::filesystem::path getDataStructureSource() const
	{
		std::ostringstream os;
		os << "structures.hpp";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getRuntimeSource() const
	{
		std::ostringstream os;
		os << "runtime.cpp";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getPythonSource() const
	{
		std::ostringstream os;
		os << "python_bindings.cpp";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						getInterfaceFolder() / os.str() ) );
	}
	
	boost::filesystem::path getAnalysisFileName() const
	{
		std::ostringstream os;
		os << "database.db";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}

	boost::filesystem::path getObjectName( const std::string& strTUName, const boost::filesystem::path& binPath ) const
	{
		std::ostringstream os;
		os << "object_" << strTUName << ".obj";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						binPath / os.str() ) );
	}
	
	boost::filesystem::path getObjectFile( const boost::filesystem::path& sourceFile, const boost::filesystem::path& binPath ) const
	{
		std::ostringstream os;
		os << "object_" << sourceFile.stem().string() << ".obj";
		return boost::filesystem::edsCannonicalise(
					boost::filesystem::absolute( 
						binPath / os.str() ) );
	}
	

	boost::filesystem::path getEGComponentSource( const std::string& strCoordinator, 
		const std::string& strHostName, const std::string& strProjectName ) const
	{
		std::ostringstream os;
		os << strCoordinator << '_' << strHostName << '_' << strProjectName << "_component.cpp";
		return boost::filesystem::edsCannonicalise(
			boost::filesystem::absolute( 
				getInterfaceFolder() / os.str() ) );
	}
	
	
private:
	boost::filesystem::path m_path;
	std::string m_projectName;
	Coordinator::PtrVector m_coordinators;
};


#endif //PROJECT_TREE_17_APRIL_2020