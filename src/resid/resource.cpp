
#include "resource.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "blueprint/factory.h"
#include "spaces/basicarea.h"
#include "spaces/basicfeature.h"
#include "spaces/blueprint.h"

#include "ed/file.hpp"

#include "common/assert_verify.hpp"

#include <boost/tokenizer.hpp>

namespace resource
{

const std::string ResourceNamespace::g_strRootNamespace = "resource";

ResourceID::~ResourceID()
{
}

const std::string ShipID::ID                    = "ship";
const std::string ShipID::ID_interior           = "interior";
const std::string ShipID::ID_interior_contour   = "interiorcontour";
const std::string ShipID::ID_exterior           = "exterior";
const std::string ShipID::ID_exterior_contour   = "exteriorcontour";

const std::string RoomID::ID = "room";
const std::string RoomID::ID_shape = "shape";



const std::string ObjectID::ID = "object";

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
                
                
                const std::string strFileName = filePath.filename().replace_extension( "" ).string();
                const std::string strFolderName = filePath.parent_path().filename().string();
                
                //naming convention
                
                using Tokeniser = boost::tokenizer< boost::char_separator< char > >;
                boost::char_separator< char > sep( "_" );
                Tokeniser tokens( strFileName, sep );
                
                Tokeniser::iterator tokenIter = tokens.begin();
                VERIFY_RTE( tokenIter != tokens.end() );
                
                std::ostringstream osUnrealResourcePath;
                osUnrealResourcePath << "/content/";
                
                if( *tokenIter == ShipID::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    
                    if( *tokenIter == ShipID::ID_interior )
                    {
                        std::ostringstream os;
                        os << strFolderName << ShipID::ID_interior;
                        ResourceID::Ptr pResourceID( new ShipID( os.str(), osUnrealResourcePath.str(), filePath, ShipID::eInterior ) );
                        pResourceTree->addResource( pResourceID );
                    }
                    else if( *tokenIter == ShipID::ID_interior_contour )
                    {
                        std::ostringstream os;
                        os << strFolderName << ShipID::ID_interior_contour;
                        ResourceID::Ptr pResourceID( new ShipID( os.str(), osUnrealResourcePath.str(), filePath, ShipID::eInteriorContour ) );
                        pResourceTree->addResource( pResourceID );
                    }
                    else if( *tokenIter == ShipID::ID_exterior )
                    {
                        std::ostringstream os;
                        os << strFolderName << ShipID::ID_exterior;
                        ResourceID::Ptr pResourceID( new ShipID( os.str(), osUnrealResourcePath.str(), filePath, ShipID::eExterior ) );
                        pResourceTree->addResource( pResourceID );
                    }
                    else if( *tokenIter == ShipID::ID_exterior_contour )
                    {
                        std::ostringstream os;
                        os << strFolderName << ShipID::ID_exterior_contour;
                        ResourceID::Ptr pResourceID( new ShipID( os.str(), osUnrealResourcePath.str(), filePath, ShipID::eExteriorContour ) );
                        pResourceTree->addResource( pResourceID );
                    }
                    else
                    {
                        THROW_RTE( "Unknown ship blueprint type found: " << filePath.string() );
                    }
                }
                else if( *tokenIter == RoomID::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    
                    if( *tokenIter == RoomID::ID_shape )
                    {
                        VERIFY_RTE( ++tokenIter != tokens.end() );
                        ResourceID::Ptr pResourceID( new RoomID( strFileName, osUnrealResourcePath.str(), filePath, true ) );
                        pResourceTree->addResource( pResourceID );
                    }
                    else
                    {
                        ResourceID::Ptr pResourceID( new RoomID( strFileName, osUnrealResourcePath.str(), filePath, false ) );
                        pResourceTree->addResource( pResourceID );
                    }
                }
                else if( *tokenIter == ObjectID::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    ResourceID::Ptr pResourceID( new ObjectID( strFileName, osUnrealResourcePath.str(), filePath ) );
                    pResourceTree->addResource( pResourceID );
                }
                else
                {
                    THROW_RTE( "Unknown blueprint type found: " << filePath.string() );
                }
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
                new ResourceID( "id", environment.expand( strFilePath ) ) );
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

void numberResources( ResourceNamespace::Ptr pResourceTree, int& id )
{
    const ResourceID::PtrVector& resources = pResourceTree->getResources();
    for( ResourceID::Ptr pResource : resources )
    {
        pResource->setID( id++ );
    }
    
    const ResourceNamespace::PtrVector& children = pResourceTree->getChildren();
    for( ResourceNamespace::Ptr pChildNamespace : children )
    {
        numberResources( pChildNamespace, id );
    }
}

resource::ResourceNamespace::Ptr load( const boost::filesystem::path& workspacePath, const std::string& strProject )
{
    Environment environment( workspacePath );
    ProjectTree projectTree( environment, workspacePath, strProject );
    
    resource::ResourceNamespace::Ptr pResourceTree( 
        new resource::ResourceNamespace( resource::ResourceNamespace::g_strRootNamespace ) );
        
    resource::recurseFolders( environment, environment.getDataFolderPath(), pResourceTree );
    
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
            resource::recurseManifest( environment, n, pResourceTree );
        }
    }
    
    int id = 1;
    numberResources( pResourceTree, id );
        
    return pResourceTree;

}

}