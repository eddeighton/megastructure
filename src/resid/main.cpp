
#include "resource.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megastructure/log.hpp"

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <map>
#include <memory>

struct Args
{
	boost::filesystem::path workspace_path;
    std::string strProject;
	bool bWait = false;
};

bool parse_args( int argc, const char* argv[], Args& args )
{
	namespace po = boost::program_options;
	try
	{
		boost::program_options::options_description options;
		boost::program_options::variables_map variables;
		
		options.add_options()
			("help", "produce help message")
			("path",  po::value< boost::filesystem::path >( &args.workspace_path ), "Workspace Path" )
            ("project", po::value< std::string >( &args.strProject ), "Project" )
			("wait",   po::bool_switch( &args.bWait ), "Wait at startup for attaching a debugger" )
		;

		po::positional_options_description p;
		p.add( "filters", -1 );

		po::store( po::command_line_parser( argc, argv).
					options( options ).
					positional( p ).run(),
					variables );
		po::notify( variables );
		
		if( variables.count("help") )
		{
			std::cout << options << "\n";
			return false;
		}
		
		if( args.workspace_path.empty() )
		{
			std::cout << "Workspace path not specified" << std::endl;
			return false;
		}
		if( args.strProject.empty() )
		{
			std::cout << "Project not specified" << std::endl;
			return false;
		}

	}
	catch( std::exception& e )
	{
		std::cout << "Invalid input. Type '--help' for options\n" << e.what() << "\n";
		return false;
	}
	return true;
}

namespace resource
{


void generateResourceHeader( ResourceNamespace::Ptr pResourceTree, std::string& strIndent, std::ostream& os )
{
    os << strIndent << "namespace " << pResourceTree->getName() << "\n";
    os << strIndent << "{\n";
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    if( pResourceTree->getName() == ResourceNamespace::g_strRootNamespace )
    {
        os << strIndent << "using ID = int;\n";
        os << strIndent << "class ResourceID;\n";
        os << strIndent << "class ResourceNamespace;\n";
    }
    
    const ResourceID::PtrVector& resources = pResourceTree->getResources();
    for( ResourceID::Ptr pResource : resources )
    {
        os << strIndent << "static const ID " << pResource->getName() << " = " << pResource->getID() << ";\n";
    }
    
    const ResourceNamespace::PtrVector& children = pResourceTree->getChildren();
    for( ResourceNamespace::Ptr pChildNamespace : children )
    {
        generateResourceHeader( pChildNamespace, strIndent, os );
    }
    
    strIndent.pop_back();
    strIndent.pop_back();
    os << strIndent << "} //" << pResourceTree->getName() << "\n";
}

void generateResourceSource( ResourceNamespace::Ptr pResourceTree, std::ostream& os )
{
    const ResourceID::PtrVector& resources = pResourceTree->getResources();
    for( ResourceID::Ptr pResource : resources )
    {
        os << "    case " << pResource->getID() << ": return TEXT(\"" << pResource->getPath() << "\");\n";
    }
    
    const ResourceNamespace::PtrVector& children = pResourceTree->getChildren();
    for( ResourceNamespace::Ptr pChildNamespace : children )
    {
        generateResourceSource( pChildNamespace, os );
    }
    
}

}

int main( int argc, const char* argv[] )
{
	Args args;
	if( !parse_args( argc, argv, args ) )
	{
		return 0;
	}
	
	if( args.bWait )
	{
		char c;
		std::cin >> c;
	}
    
    try 
    {
        Environment environment( args.workspace_path );
        ProjectTree projectTree( environment, args.workspace_path, args.strProject );
            
        //configure log
        //auto logThreadPool = megastructure::configureLog( environment.getLogFolderPath(), "resid" );
        
        resource::ResourceNamespace::Ptr pResourceTree =
            resource::load( args.workspace_path, args.strProject );
            
        {
            std::string strIndent;
            std::ostringstream osResourceHeader;
            resource::generateResourceHeader( pResourceTree, strIndent, osResourceHeader );
            boost::filesystem::updateFileIfChanged( 
                projectTree.getResourceHeader(), osResourceHeader.str() );
        }
        {
            std::ostringstream os;
            os << "#include \"interface/" << projectTree.getProjectName() << "/resource.hpp\"\n";
            os << "#include \"CoreMinimal.h\"\n";
            os << "\n";
            os << "namespace resource\n";
            os << "{\n";
            os << "const TCHAR* getResource( ID iResourceID )\n";
            os << "{\n";
            os << "  switch( iResourceID )\n";
            os << "  {\n";
            resource::generateResourceSource( pResourceTree, os );
            os << "    default: return nullptr;\n";
            os << "  }\n";
            os << "}\n";
            os << "}\n";
            boost::filesystem::updateFileIfChanged( 
                projectTree.getResourceSource(), os.str() );
        }
        
        spdlog::drop_all(); 
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log configuration failed" << std::endl;
        return 1;
    }
    catch( std::runtime_error& ex )
    {
        std::cout << "Runtime error: " << ex.what() << std::endl;
        return 1;
    }
    
    return 0;
}
