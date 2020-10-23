
#include "resource.hpp"
#include "blueprint.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "blueprint/factory.h"
#include "spaces/basicarea.h"
#include "spaces/basicfeature.h"
#include "spaces/blueprint.h"

#include "ed/file.hpp"

#include "common/assert_verify.hpp"


namespace resource
{

const std::string ResourceNamespace::g_strRootNamespace = "resource";

ResourceID::~ResourceID()
{
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

void generateShipBlueprintResources( const ShipNamespace* pShipNamespace, resource::ResourceNamespace::Ptr pResourceTree )
{
    ResourceID::Ptr pResourceID( 
        new ResourceID( "id", "/someship.thing" ) );
    pResourceTree->addResource( pResourceID );
    
    const BlueprintResource::PtrVector& blueprints = pShipNamespace->getBlueprints();
    for( BlueprintResource::Ptr pResource : blueprints )
    {
        if( const ShipBlueprint* pShip = dynamic_cast< const ShipBlueprint* >( pResource.get() ) )
        {
        }
        else
        {
            THROW_RTE( "Unexpected ship blueprint: " << pResource->getBlueprintPath().string() );
        }
        
    }
}

void recurseBlueprints( resource::BlueprintNamespace::Ptr pBlueprintNamespace, resource::ResourceNamespace::Ptr pResourceTree )
{
    const BlueprintResource::PtrVector& blueprints = pBlueprintNamespace->getBlueprints();
    for( BlueprintResource::Ptr pResource : blueprints )
    {
        if( const RoomBlueprint* pRoom = dynamic_cast< const RoomBlueprint* >( pResource.get() ) )
        {
        }
        else if( const ObjectBlueprint* pObject = dynamic_cast< const ObjectBlueprint* >( pResource.get() ) )
        {
        }
        else
        {
            THROW_RTE( "Unexpected blueprint: " << pResource->getBlueprintPath().string() );
        }
        
    }
    
    const BlueprintNamespace::PtrVector& children = pBlueprintNamespace->getChildren();
    for( BlueprintNamespace::Ptr pChildNamespace : children )
    {
        ResourceNamespace::Ptr pNestedNamespace( 
            new ResourceNamespace( pChildNamespace->getName() ) );
        pResourceTree->addNamespace( pNestedNamespace );
        
        if( const ShipNamespace* pShipNamespace = dynamic_cast< const ShipNamespace* >( pChildNamespace.get() ) )
        {
            generateShipBlueprintResources( pShipNamespace, pNestedNamespace );
        }
        else
        {
            recurseBlueprints( pChildNamespace, pNestedNamespace );
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
        
    resource::BlueprintNamespace::Ptr pBlueprintNamespace =
        loadBlueprints( workspacePath, strProject );
        
    recurseBlueprints( pBlueprintNamespace, pResourceTree );
    
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