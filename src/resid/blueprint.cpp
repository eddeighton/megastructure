
#include "blueprint.hpp"
#include "resource.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "blueprint/factory.h"
#include "spaces/basicarea.h"
#include "spaces/basicfeature.h"
#include "spaces/blueprint.h"

#include "ed/file.hpp"

#include <boost/tokenizer.hpp>

namespace resource
{

const std::string ShipBlueprint::ID             = "ship";
const std::string ShipBlueprint::ID_interior    = "interior";
const std::string ShipBlueprint::ID_exterior    = "exterior";
const std::string ShipBlueprint::ID_contour     = "contour";
const std::string ShipBlueprint::ID_whole       = "whole";

const std::string RoomBlueprint::ID             = "room";
const std::string RoomBlueprint::ID_shape       = "shape";

const std::string ObjectBlueprint::ID           = "object";

BlueprintResource::~BlueprintResource()
{ 
}
BlueprintNamespace::~BlueprintNamespace()
{
}
    
void recurseFolders( const Environment& environment, const boost::filesystem::path& folderPath, 
        BlueprintNamespace::PtrVector& namespaces, BlueprintResource::PtrVector& blueprints )
{
    for( auto& directoryItem : boost::filesystem::directory_iterator( folderPath ) )
    {
        if( boost::filesystem::is_directory( directoryItem ) )
        {
            const boost::filesystem::path filePath = directoryItem;
            const std::string strIdentity = filePath.stem().string();
            
            BlueprintNamespace::Ptr pNestedNamespace;
            {
                BlueprintNamespace::PtrVector nestedNamespaces;
                BlueprintResource::PtrVector nestedBlueprints;
                recurseFolders( environment, directoryItem, nestedNamespaces, nestedBlueprints );
                
                if( !nestedBlueprints.empty() )
                {
                    bool bShip = false;
                    bool bOther = false;
                    for( BlueprintResource::Ptr pBlueprint : nestedBlueprints )
                    {
                        if( dynamic_cast< const ShipBlueprint* >( pBlueprint.get() ) )
                            bShip = true;
                        else
                            bOther = true;
                    }
                    VERIFY_RTE_MSG( !( bShip && bOther ), "Ship folder has other blueprints: " << filePath.string() );
                    if( bShip )
                    {
                        pNestedNamespace.reset( new ShipNamespace( strIdentity ) );
                    }
                    else
                    {
                        pNestedNamespace.reset( new BlueprintNamespace( strIdentity ) );
                    }
                    pNestedNamespace->m_blueprints = nestedBlueprints;
                }
                else
                {
                    pNestedNamespace.reset( new BlueprintNamespace( strIdentity ) );
                }
                
                pNestedNamespace->m_children = nestedNamespaces;
            }
            namespaces.push_back( pNestedNamespace );
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
                
                if( *tokenIter == ShipBlueprint::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    
                    if( *tokenIter == ShipBlueprint::ID_interior )
                    {
                        ++tokenIter;
                        if( tokenIter == tokens.end() )
                        {
                            BlueprintResource::Ptr pResourceID( new ShipBlueprint( filePath, ShipBlueprint::eInterior ) );
                            blueprints.push_back( pResourceID );
                        }
                        else if( *tokenIter == ShipBlueprint::ID_contour )
                        {
                            BlueprintResource::Ptr pResourceID( new ShipBlueprint( filePath, ShipBlueprint::eInteriorContour ) );
                            blueprints.push_back( pResourceID );
                            VERIFY_RTE_MSG( ++tokenIter == tokens.end(), "Incorrect blueprint type found: " << filePath.string() );
                        }
                        else
                        {
                            THROW_RTE( "Unknown ship blueprint type found: " << filePath.string() );
                        }
                    }
                    else if( *tokenIter == ShipBlueprint::ID_exterior )
                    {
                        ++tokenIter;
                        if( tokenIter == tokens.end() )
                        {
                            BlueprintResource::Ptr pResourceID( new ShipBlueprint( filePath, ShipBlueprint::eExterior ) );
                            blueprints.push_back( pResourceID );
                        }
                        else if( *tokenIter == ShipBlueprint::ID_contour )
                        {
                            BlueprintResource::Ptr pResourceID( new ShipBlueprint( filePath, ShipBlueprint::eExteriorContour ) );
                            blueprints.push_back( pResourceID );
                            VERIFY_RTE_MSG( ++tokenIter == tokens.end(), "Incorrect blueprint type found: " << filePath.string() );
                        }
                        else
                        {
                            THROW_RTE( "Unknown ship blueprint type found: " << filePath.string() );
                        }
                    }
                    else if( *tokenIter == ShipBlueprint::ID_whole )
                    {
                        //ignor
                    }
                    else
                    {
                        THROW_RTE( "Unknown ship blueprint type found: " << filePath.string() );
                    }
                }
                else if( *tokenIter == RoomBlueprint::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    
                    if( *tokenIter == RoomBlueprint::ID_shape )
                    {
                        VERIFY_RTE( ++tokenIter != tokens.end() );
                        BlueprintResource::Ptr pResourceID( new RoomBlueprint( filePath, true ) );
                        blueprints.push_back( pResourceID );
                    }
                    else
                    {
                        BlueprintResource::Ptr pResourceID( new RoomBlueprint( filePath, false ) );
                        blueprints.push_back( pResourceID );
                    }
                }
                else if( *tokenIter == ObjectBlueprint::ID )
                {
                    VERIFY_RTE( ++tokenIter != tokens.end() );
                    BlueprintResource::Ptr pResourceID( new ObjectBlueprint( filePath ) );
                    blueprints.push_back( pResourceID );
                }
                else
                {
                    THROW_RTE( "Unknown blueprint type found: " << filePath.string() );
                }
            }
        }
    }
}

resource::BlueprintNamespace::Ptr loadBlueprints( const boost::filesystem::path& workspacePath, const std::string& strProject )
{
    Environment environment( workspacePath );
    ProjectTree projectTree( environment, workspacePath, strProject );
    
    resource::BlueprintNamespace::Ptr pBlueprintNamespace( 
        new resource::BlueprintNamespace( resource::ResourceNamespace::g_strRootNamespace ) );
        
    resource::recurseFolders( environment, environment.getDataFolderPath(), 
        pBlueprintNamespace->m_children, pBlueprintNamespace->m_blueprints );
    
    return pBlueprintNamespace;

}


}