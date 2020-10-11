

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megastructure/log.hpp"

#include "blueprint/factory.h"
#include "spaces/basicarea.h"
#include "spaces/basicfeature.h"
#include "spaces/blueprint.h"

#include "ed/file.hpp"

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>

#include <iostream>
#include <map>

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




void recurseFolders( const boost::filesystem::path& folderPath, int iDepth, std::ostream& os )
{
    os << std::string( iDepth * 2, '#' ) << folderPath.string() << "\n";
    
    for( auto& directoryItem : boost::filesystem::directory_iterator( folderPath ) )
    {
        if( boost::filesystem::is_directory( directoryItem ) )
        {
            recurseFolders( directoryItem, iDepth + 1, os );
        }
        else if( boost::filesystem::is_regular_file( directoryItem ) )
        {
            if( boost::filesystem::extension( directoryItem ) == ".blu" )
            {
                const boost::filesystem::path filePath = directoryItem;
                Blueprint::Factory factory;
                Blueprint::Site::Ptr pTestSite = factory.create( filePath.string() );
                VERIFY_RTE_MSG( pTestSite, "Failed to load blueprint: " << filePath.string() );
                
                Blueprint::Blueprint::Ptr pBlueprint = 
                    boost::dynamic_pointer_cast< Blueprint::Blueprint >( pTestSite );
                VERIFY_RTE_MSG( pBlueprint, "Failed to get blueprint: " << filePath.string() );
                
                os << filePath.string() << "\n";
                
            }
        }
    }
}

void recurseManifest( const Ed::Node& node, std::ostream& os )
{
    os << "Node: " << node.statement.declarator.identifier.get() << "\n";
    
    for( const Ed::Node& n : node.children )
    {
        recurseManifest( n, os );
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
        auto logThreadPool = megastructure::configureLog( environment.getLogFolderPath(), "resid" );
    
        //spdlog::info( "resid tool started with pid:{}", Common::getProcessID() );
    
        const boost::filesystem::path& dataFolderPath = environment.getDataFolderPath();
        
        //spdlog::info( "Analysing data for workspace: {} project: {} data folder: {}", 
        //    args.workspace_path.string(), args.strProject, dataFolderPath.string() );
            
        std::ostringstream os;
            
        recurseFolders( dataFolderPath, 1, os );
        
        {
            Ed::Node manifest;
            {
                Ed::BasicFileSystem filesystem;
                Ed::File edFile( filesystem, projectTree.getManifestFile().string() );
                edFile.expandShorthand();
                edFile.removeTypes();
                edFile.toNode( manifest );
            }
            for( const Ed::Node& n : manifest )
            {
                recurseManifest( n, os );
            }
        }
            
        
        boost::filesystem::updateFileIfChanged( 
            projectTree.getResourceHeader(), os.str() );
        
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
