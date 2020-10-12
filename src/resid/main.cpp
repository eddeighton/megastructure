

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

class ResourceID
{
public:
    using Ptr = std::shared_ptr< ResourceID >;
    using PtrVector = std::vector< Ptr >;
    
    ResourceID( const std::string& strPath )
        :   m_strPath( strPath )
    {}
    
    const std::string& getPath() const { return m_strPath; }
    
protected:
    std::string m_strPath;
};

class ResourceIDRange : public ResourceID
{
public:
    ResourceIDRange( const std::string& strPath, std::size_t szSize )
        :   ResourceID( strPath ),
            m_szSize( szSize )
    {}
    
protected:
    std::size_t m_szSize;
};

class ResourceNamespace
{
public:
    using Ptr = std::shared_ptr< ResourceNamespace >;
    using PtrVector = std::vector< Ptr >;
    
    ResourceNamespace( const std::string& strName )
        :   m_strName( strName )
    {
        
    }

    void addNamespace( Ptr pNamespace ) { m_children.push_back( pNamespace ); }
    void addResource( ResourceID::Ptr pResource ) { m_resources.push_back( pResource ); }
    
    const std::string& getName() const { return m_strName; }
    const PtrVector& getChildren() const { return m_children; }
    const ResourceID::PtrVector& getResources() const { return m_resources; }
    
private:
    std::string m_strName;
    PtrVector m_children;
    ResourceID::PtrVector m_resources;
};


void recurseFolders( const Environment& environment, const boost::filesystem::path& folderPath, ResourceNamespace::Ptr pResourceTree )
{
    for( auto& directoryItem : boost::filesystem::directory_iterator( folderPath ) )
    {
        if( boost::filesystem::is_directory( directoryItem ) )
        {
            const boost::filesystem::path filePath = directoryItem;
            const std::string strIdentity = filePath.stem().string();
            
            ResourceNamespace::Ptr pNestedNamespace( 
                new ResourceNamespace( strIdentity ) );
            pResourceTree->addNamespace( pNestedNamespace );
            
            recurseFolders( environment, directoryItem, pNestedNamespace );
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
                
                
            }
        }
    }
}

void recurseManifest( const Environment& environment, const Ed::Node& node, ResourceNamespace::Ptr pResourceTree )
{
    const std::string strIdentity = node.statement.declarator.identifier.get();
    
    if( node.children.empty() )
    {
        if( strIdentity == "id" )
        {
            std::string strFilePath;
            Ed::IShorthandStream is( node.statement.shorthand.get() );
            is >> strFilePath;
            
            ResourceID::Ptr pResourceID( 
                new ResourceID( environment.expand( strFilePath ) ) );
            pResourceTree->addResource( pResourceID );
        }
        else if( strIdentity == "range" )
        {
            THROW_RTE( "Unsupported range resource specifier: " << node );
        }
        else
        {
            THROW_RTE( "Unknown resource specifier: " << node );
        }
    }
    else
    {
        ResourceNamespace::Ptr pNestedNamespace( 
            new ResourceNamespace( strIdentity ) );
        pResourceTree->addNamespace( pNestedNamespace );
    
        for( const Ed::Node& n : node.children )
        {
            recurseManifest( environment, n, pNestedNamespace );
        }
    }
}

void generateResourceHeader( ResourceNamespace::Ptr pResourceTree, std::string& strIndent, int& resID, std::ostream& os )
{
    os << strIndent << "namespace " << pResourceTree->getName() << "\n";
    os << strIndent << "{\n";
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    
    const ResourceID::PtrVector& resources = pResourceTree->getResources();
    for( ResourceID::Ptr pResource : resources )
    {
        os << strIndent << "static const int ID = " << resID++ << ";\n";
    }
    
    const ResourceNamespace::PtrVector& children = pResourceTree->getChildren();
    for( ResourceNamespace::Ptr pChildNamespace : children )
    {
        generateResourceHeader( pChildNamespace, strIndent, resID, os );
    }
    
    strIndent.pop_back();
    strIndent.pop_back();
    os << strIndent << "} //" << pResourceTree->getName() << "\n";
}

void generateResourceSource( ResourceNamespace::Ptr pResourceTree, int& resID, std::ostream& os )
{
    const ResourceID::PtrVector& resources = pResourceTree->getResources();
    for( ResourceID::Ptr pResource : resources )
    {
        os << "    case " << resID++ << ": return TEXT(\"" << pResource->getPath() << "\");\n";
    }
    
    const ResourceNamespace::PtrVector& children = pResourceTree->getChildren();
    for( ResourceNamespace::Ptr pChildNamespace : children )
    {
        generateResourceSource( pChildNamespace, resID, os );
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
        const boost::filesystem::path& dataFolderPath = environment.getDataFolderPath();
        
        ResourceNamespace::Ptr pResourceTree( new ResourceNamespace( "res" ) );
            
        recurseFolders( environment, dataFolderPath, pResourceTree );
        
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
                recurseManifest( environment, n, pResourceTree );
            }
        }
            
            
        {
            std::string strIndent;
            int resID = 1;
            std::ostringstream osResourceHeader;
            generateResourceHeader( pResourceTree, strIndent, resID, osResourceHeader );
            boost::filesystem::updateFileIfChanged( 
                projectTree.getResourceHeader(), osResourceHeader.str() );
        }
        {
            std::ostringstream os;
            os << "#include \"interface/" << projectTree.getProjectName() << "/resource.hpp\"\n";
            os << "#include \"CoreMinimal.h\"\n";
            os << "\n";
            os << "const TCHAR* getResource( int iResourceID )\n";
            os << "{\n";
            os << "  switch( iResourceID )\n";
            os << "  {\n";
            int resID = 1;
            generateResourceSource( pResourceTree, resID, os );
            os << "    default: return nullptr;\n";
            os << "  }\n";
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
