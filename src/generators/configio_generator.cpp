
#include <ostream>

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/codegen/dataAccessPrinter.hpp"
#include "eg_compiler/input.hpp"
#include "eg_compiler/allocator.hpp"
#include "eg_compiler/layout.hpp"
#include "eg_compiler/derivation.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/filesystem.hpp>

struct ConstTree
{
    using Ptr = std::shared_ptr< ConstTree >;
    using Vector = std::vector< Ptr >;
    using ConstantVector = std::vector< const eg::concrete::Dimension_User* >;
    const eg::concrete::Action* pAction;
    Vector children;
    ConstantVector constants;
};

const eg::concrete::Action* findComponentRoot( const ProjectTree& projectTree, const eg::concrete::Action* pInterfaceRoot )
{
    VERIFY_RTE( pInterfaceRoot );
    {
        const eg::interface::Root* pRoot = dynamic_cast< const eg::interface::Root* >( pInterfaceRoot->getContext() );
        VERIFY_RTE( pRoot );
        VERIFY_RTE( pRoot->getRootType() == eg::eInterfaceRoot );
    }
    
    VERIFY_RTE( pInterfaceRoot->getChildren().size() == 1U );
    const eg::concrete::Action* pMegaRoot = 
        dynamic_cast< const eg::concrete::Action* >( pInterfaceRoot->getChildren().front() );
    {
        const eg::interface::Root* pRoot = dynamic_cast< const eg::interface::Root* >( pMegaRoot->getContext() );
        VERIFY_RTE( pRoot );
        VERIFY_RTE( pRoot->getRootType() == eg::eMegaRoot );
    }
    
    for( const eg::concrete::Element* pCoordinatorElement : pMegaRoot->getChildren() )
    {
        if( const eg::concrete::Action* pCoordinatorRoot = 
                dynamic_cast< const eg::concrete::Action* >( pCoordinatorElement ) )
        {
            const eg::interface::Root* pCoordinatorInterfaceRoot =
                dynamic_cast< const eg::interface::Root* >( pCoordinatorRoot->getContext() );
            {
                VERIFY_RTE( pCoordinatorInterfaceRoot );
                VERIFY_RTE( pCoordinatorInterfaceRoot->getRootType() == eg::eCoordinator );
            }
            if( pCoordinatorInterfaceRoot->getIdentifier() == projectTree.getCoordinatorName() )
            {
                for( const eg::concrete::Element* pHostNameElement : pCoordinatorRoot->getChildren() )
                {
                    if( const eg::concrete::Action* pHostNameRoot = 
                            dynamic_cast< const eg::concrete::Action* >( pHostNameElement ) )
                    {
                        const eg::interface::Root* pInterfaceHostNameRoot =
                            dynamic_cast< const eg::interface::Root* >( pHostNameRoot->getContext() );
                        {
                            VERIFY_RTE( pInterfaceHostNameRoot );
                            VERIFY_RTE( pInterfaceHostNameRoot->getRootType() == eg::eHostName );
                        }
                        if( pInterfaceHostNameRoot->getIdentifier() == projectTree.getHostName() )
                        {
                            for( const eg::concrete::Element* pProjectElement : pHostNameRoot->getChildren() )
                            {
                                if( const eg::concrete::Action* pProjectRoot = 
                                        dynamic_cast< const eg::concrete::Action* >( pProjectElement ) )
                                {
                                    const eg::interface::Root* pInterfaceProjectRoot =
                                        dynamic_cast< const eg::interface::Root* >( pProjectRoot->getContext() );
                                    {
                                        VERIFY_RTE( pInterfaceProjectRoot );
                                        VERIFY_RTE( pInterfaceProjectRoot->getRootType() == eg::eProjectName );
                                    }
                                    if( pInterfaceProjectRoot->getIdentifier() == projectTree.getProjectName() )
                                    {
                                        //found it...
                                        return pProjectRoot;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return nullptr;
}

void buildTree( const eg::concrete::Action* pAction, ConstTree::Ptr pTree )
{
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Dimension_User* pUserDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( pElement ) )
        {
            if( pUserDimension->getDimension()->isConst() )
            {
                pTree->constants.push_back( pUserDimension );
            }
        }
        else if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            ConstTree::Ptr pNestedTree( new ConstTree{ pNestedAction } );
            pTree->children.push_back( pNestedTree );
            buildTree( pNestedAction, pNestedTree );
        }
    }
}

bool pruneTree( ConstTree::Ptr pTree )
{
    bool bChildHasConstants = false;
    
    ConstTree::Vector::iterator i = pTree->children.begin();
    while( i != pTree->children.end() )
    {
        ConstTree::Ptr pChild = *i;
        
        if( pruneTree( pChild ) )
        {
            i = pTree->children.erase( i );
        }
        else
        {
            bChildHasConstants = true;
            ++i;
        }
    }
    
    if( !bChildHasConstants && pTree->constants.empty() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

using ObjectTreeMap = std::map< const eg::interface::Object*, ConstTree::Ptr >;
    
bool gatherObjectTrees( ObjectTreeMap& objectTreeMap, ConstTree::Ptr pTree )
{
    ConstTree::Vector::iterator i = pTree->children.begin();
    while( i != pTree->children.end() )
    {
        ConstTree::Ptr pChild = *i;
        
        if( gatherObjectTrees( objectTreeMap, pChild ) )
        {
            i = pTree->children.erase( i );
        }
        else
        {
            ++i;
        }
    }
    
    if( const eg::interface::Object* pInterfaceObject =
        dynamic_cast< const eg::interface::Object* >( pTree->pAction->getContext() ) )
    {
        if( !pruneTree( pTree ) )
        {
            objectTreeMap.insert( std::make_pair( pInterfaceObject, pTree ) );
        }
        return true;
    }
    else
    {
        return false;
    }
}

std::string computeIndexString( const eg::interface::Object* pObject, const eg::concrete::Action* pAction, int iNodeCount )
{
    std::ostringstream os;
    
    os << " ( index_" << iNodeCount-- << " ) ";
    while( pAction->getContext() != pObject )
    {
        os << " + ( index_" << iNodeCount-- << " * " << pAction->getLocalDomainSize() << " ) ";
        pAction = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
    }
    
    return os.str();
}

void generateLoadRecurse( std::ostream& os, const eg::Layout& layout, eg::PrinterFactory& printerFactory, 
        const eg::interface::Object* pObject, ConstTree::Ptr pTree, std::string& strIndent, int iNodeCount )
{
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    
    os << strIndent << "for( const Ed::Node& n" << ( iNodeCount + 1 ) << " : n" << iNodeCount << ".children )\n";
    os << strIndent << "{\n";
    os << strIndent << "  std::string identity_" << ( iNodeCount + 1 ) << ";\n";
    os << strIndent << "  int index_" << ( iNodeCount + 1 ) << ";\n";
    os << strIndent << "  if( parseIdentifier( n" << ( iNodeCount + 1 ) << ".statement.declarator.identifier.get(), identity_" << ( iNodeCount + 1 ) << ", index_" << ( iNodeCount + 1 ) << " ) )\n";
    os << strIndent << "  {\n";
    
    os << strIndent << "    if( identity_" << ( iNodeCount + 1 ) << " == \"\" )\n";
    os << strIndent << "    {\n";
    os << strIndent << "      ERR( \"Node missing identifier\" );\n";
    os << strIndent << "    }\n";
        
    for( const eg::concrete::Dimension_User* pConstant : pTree->constants )
    {
    os << strIndent << "    //load: " << pConstant->getDimension()->getFriendlyName() << "\n";
    os << strIndent << "    else if( identity_" << ( iNodeCount + 1 ) << " == \"" << pConstant->getDimension()->getIdentifier() << "\" )\n";
    os << strIndent << "    {\n";
    const eg::DataMember* pDataMember = layout.getDataMember( pConstant ); 
    os << strIndent << "      Ed::IShorthandStream is( n" << ( iNodeCount + 1 ) << ".statement.shorthand.get() );\n";
    os << strIndent << "      is >> " << *printerFactory.write( pDataMember, computeIndexString( pObject, pTree->pAction, iNodeCount ).c_str() ) << ";\n";
    os << strIndent << "    }\n";
}
    for( ConstTree::Ptr pChild : pTree->children )
    {
    os << strIndent << "    else if( identity_" << ( iNodeCount + 1 ) << " == \"" << pChild->pAction->getContext()->getIdentifier() << "\" )\n";
    os << strIndent << "    {\n";
    generateLoadRecurse( os, layout, printerFactory, pObject, pChild, strIndent, iNodeCount + 1 );
    os << strIndent << "    }\n";
    }
    
    os << strIndent << "    else\n";
    os << strIndent << "    {\n";
    os << strIndent << "      ERR( \"Failed to match node: \" << identity_" << ( iNodeCount + 1 ) << " );\n";
    os << strIndent << "    }\n";
    os << strIndent << "  }\n";
    os << strIndent << "  else\n";
    os << strIndent << "  {\n";
    os << strIndent << "    ERR( \"Failed to match node: \" << identity_" << ( iNodeCount + 1 ) << " );\n";
    os << strIndent << "  }\n";
    os << strIndent << "}\n";
    
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
}

void generateLoad( std::ostream& os, const eg::Layout& layout, eg::PrinterFactory& printerFactory, 
        const eg::interface::Object* pObject, ConstTree::Ptr pTree  )
{
    //attempt to load ed file for object
    os << " {\n";
    os << "  boost::filesystem::path edFilePath = sourceFolderPath / \"" << pObject->getIdentifier() << ".ed\";\n";
    os << "  if( boost::filesystem::exists( edFilePath ) )\n";
    os << "  {\n";
    os << "    try\n";
    os << "    {\n";
    os << "      Ed::Node node;\n";
    os << "      {\n";
    os << "        Ed::BasicFileSystem filesystem;\n";
    os << "        Ed::File edFile( filesystem, edFilePath.string() );\n";
    os << "        edFile.expandShorthand();\n";
    os << "        edFile.removeTypes();\n";
    os << "        edFile.toNode( node );\n";
    os << "      }\n";
    os << "      for( const Ed::Node& n : node.children )\n";
    os << "      {\n";
    os << "      for( const Ed::Node& n1 : n.children )\n";
    os << "      {\n";
    os << "        std::string identity_1;\n";
    os << "        int index_1;\n";
    os << "        if( parseIdentifier( n1.statement.declarator.identifier.get(), identity_1, index_1 ) )\n";
    os << "        {\n";
    os << "          if( identity_1 == \"\" )\n";
    os << "          {\n";
    os << "            ERR( \"Node missing identifier\" );\n";
    os << "          }\n";
    os << "          else if( identity_1 == \"" << pTree->pAction->getContext()->getIdentifier() << "\" )\n";
    os << "          {\n";
    std::string strIndent = "      ";
    generateLoadRecurse( os, layout, printerFactory, pObject, pTree, strIndent, 1 );
    os << "          }\n";
    os << "          else\n";
    os << "          {\n";
    os << "            ERR( \"Failed to match node: \" << identity_1 );\n";
    os << "          }\n";
    os << "        }\n";
    os << "        else\n";
    os << "        {\n";
    os << "          ERR( \"Failed to match node: \" << identity_1 );\n";
    os << "        }\n";
    os << "        }\n";
    os << "      }\n";
    os << "    }\n";
    os << "    catch( std::exception& ex )\n";
    os << "    {\n";
    os << "      ERR( \"Error loading config file: \" << edFilePath.string() << \" : \" << ex.what() );\n";
    os << "    }\n";
    os << "  }\n";
    os << "  else\n";
    os << "  {\n";
    os << "    ERR( \"Config file not found: \" << edFilePath.string() );\n";
    os << "  }\n";
    os << " }\n";
}

void generateSaveRecurse( std::ostream& os, const eg::Layout& layout, eg::PrinterFactory& printerFactory, 
        const eg::interface::Object* pObject, ConstTree::Ptr pTree, std::string& strIndent, int iNodeCount )
{
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    
    os << strIndent << "for( int index_" << ( iNodeCount + 1 ) << " = 0; index_" << ( iNodeCount + 1 ) << " != " << pTree->pAction->getLocalDomainSize() << "; ++index_" << ( iNodeCount + 1 ) << " )\n";
    os << strIndent << "{\n";
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' );
    os << strIndent << "Ed::Node n" << ( iNodeCount + 1 ) << ";\n";
    os << strIndent << "n" << ( iNodeCount + 1 ) << ".statement.declarator.identifier = makeIdentifier( \"" << 
        pTree->pAction->getContext()->getIdentifier() << "\", index_" << ( iNodeCount + 1 ) << ");\n";
        
    for( const eg::concrete::Dimension_User* pConstant : pTree->constants )
    {
    os << strIndent << "//save: " << pConstant->getDimension()->getFriendlyName() << "\n";
    os << strIndent << "{\n";
    //os << strIndent << "  int index_" << ( iNodeCount + 1 ) << " = 0;\n";
    os << strIndent << "  Ed::Node dimNode;\n";
    os << strIndent << "  dimNode.statement.declarator.identifier = makeIdentifier( \"" << 
        pConstant->getDimension()->getIdentifier() << "\", 0);\n";
    const eg::DataMember* pDataMember = layout.getDataMember( pConstant );            
    os << strIndent << "  if( !dimNode.statement.shorthand )\n";
    os << strIndent << "    dimNode.statement.shorthand = Ed::Shorthand();\n";
    os << strIndent << "  Ed::OShorthandStream os( dimNode.statement.shorthand.get() );\n";
    os << strIndent << "  os << " << *printerFactory.read( pDataMember, computeIndexString( pObject, pTree->pAction, iNodeCount + 1 ).c_str() ) << ";\n";
    os << strIndent << "  n" << ( iNodeCount + 1 ) << ".children.push_back( dimNode );\n";
    os << strIndent << "}\n";
    }
    
    for( ConstTree::Ptr pChild : pTree->children )
    {
    os << strIndent << "{\n";
    generateSaveRecurse( os, layout, printerFactory, pObject, pChild, strIndent, iNodeCount + 1 );
    os << strIndent << "}\n";
    }
    
    os << strIndent << "n" << iNodeCount << ".children.push_back( n" << ( iNodeCount + 1 ) << " );\n";
    
    strIndent.pop_back();
    strIndent.pop_back();
    os << strIndent << "}\n";
    strIndent.pop_back();
    strIndent.pop_back();
}

void generateSave( std::ostream& os, const eg::Layout& layout, eg::PrinterFactory& printerFactory, 
        const eg::interface::Object* pObject, ConstTree::Ptr pTree  )
{
    os << "{\n";
    os << "    Ed::Node n1( Ed::Statement( Ed::Declarator( \"" << pObject->getIdentifier() << "\" ) ) );\n";
    {
    //os << "    for( int index_1 = 0; index_1 != " << pTree->pAction->getLocalDomainSize() << "; ++index_1 )\n";
    os << "    {\n";
        std::string strIndent = "    ";
        generateSaveRecurse( os, layout, printerFactory, pObject, pTree, strIndent, 1 );
    os << "    }\n";
    }
    os << "    boost::filesystem::path edFilePath = sourceFolderPath / \"" << pObject->getIdentifier() << ".ed\";\n";
    os << "    std::ofstream os( edFilePath.string() );\n";
    os << "    os << n1;\n";
    os << "}\n";
    
}

void generateConfigIO( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, 
        eg::PrinterFactory::Ptr pPrinterFactory )
{
    
    const eg::concrete::Action*     pConcreteRoot       = session.getInstanceRoot();
    const eg::Layout&               layout              = session.getLayout();
    const eg::LinkAnalysis&         linkAnaysis         = session.getLinkAnalysis();
    const eg::DerivationAnalysis&   derivationAnalysis  = session.getDerivationAnalysis();
    
    const eg::concrete::Action* pComponentRoot = findComponentRoot( projectTree, pConcreteRoot );
    VERIFY_RTE_MSG( pComponentRoot, "Could not find component root" );
    
    ObjectTreeMap objectTreeMap;
    {
        ConstTree::Ptr pTree( new ConstTree{ pComponentRoot } );
        buildTree( pComponentRoot, pTree );
        
        bool bRootIsObject = gatherObjectTrees( objectTreeMap, pTree );
        VERIFY_RTE( bRootIsObject );
        
    }
    
    
    os << "#include \"geometry/config_conversions.hpp\"\n";
    os << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
    os << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
    os << "#include \"ed/file.hpp\"\n";
    os << "#include <fstream>\n";
    os << "\n";
    os << "inline bool parseIdentifier( const Ed::Identifier& identifier, std::string& strID, int& index )\n";
    os << "{\n";
    os << "    if( identifier.size() > 5 )\n";
    os << "    {\n";
    os << "        if( identifier[ identifier.size() - 5U ] == '_' )\n";
    os << "        {\n";
    os << "            if( std::isdigit( identifier[ identifier.size() - 1U ] ) &&\n";
    os << "                std::isdigit( identifier[ identifier.size() - 2U ] ) &&\n";
    os << "                std::isdigit( identifier[ identifier.size() - 3U ] ) &&\n";
    os << "                std::isdigit( identifier[ identifier.size() - 4U ] ) )\n";
    os << "            {\n";
    os << "                strID.assign( identifier.begin(), identifier.begin() + identifier.size() - 5U );\n";
    os << "                index = stoi( identifier.substr( identifier.size() - 4U, 4U ) );\n";
    os << "                return true;\n";
    os << "            }\n";
    os << "        }\n";
    os << "    }\n";
    os << "    return false;\n";
    os << "}\n";
    os << "\n";
    os << "inline std::string makeIdentifier( const Ed::Identifier& identifier, int index )\n";
    os << "{\n";
    os << "    std::ostringstream os;\n";
    os << "    os << identifier << '_' << std::setw( 4 ) << std::setfill( '0' ) << index;\n";
    os << "    return os.str();\n";
    os << "}\n";
    os << "\n";
    
    os << "void config_io_load( const char* pszSrcFolderPath )\n";
    os << "{\n";
    os << "  const boost::filesystem::path sourceFolderPath = pszSrcFolderPath;\n";
    os << "  LOG( \"config_io_load: \" << sourceFolderPath.string() );\n";
    
    for( auto& i : objectTreeMap )
    {
        generateLoad( os, layout, *pPrinterFactory, i.first, i.second );
    }
    
    os << "}\n";
    os << "void config_io_save( const char* pszSrcFolderPath )\n";
    os << "{\n";
    os << "  const boost::filesystem::path sourceFolderPath = pszSrcFolderPath;\n";
    os << "  LOG( \"config_io_save: \" << sourceFolderPath.string() );\n";
    
    for( auto& i : objectTreeMap )
    {
        generateSave( os, layout, *pPrinterFactory, i.first, i.second );
    }
    
    //os << "  Ed::Node n1( Ed::Statement( Ed::Declarator( \"config\" ) ) );\n";
    //os << "  std::ofstream os( pszFilePath );\n";
    //os << "  os << n1;\n";
    os << "}\n";
}