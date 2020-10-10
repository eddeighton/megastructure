
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

void generateGeometryInterface( std::ostream& os, const Environment& environment )
{
    os << "#ifndef GEOMETRY_INTERFACE\n";
    os << "#define GEOMETRY_INTERFACE\n";
    
    os << "\n\n";
    os << "void load_config( const std::string& filepath );\n";
    os << "void save_config( const std::string& filepath );\n";
    os << "eg::Event convert_string_to_config_reference( const std::string& strReference );\n";
    
    os << "#endif //GEOMETRY_INTERFACE\n";
}

void recurseLoadTree( std::ostream& os,
        eg::PrinterFactory& printerFactory, 
        const eg::Layout& layout, 
        const eg::concrete::Action* pAction, 
        int iNodeCount, 
        std::string& strIndent )
{
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' );
    
    os << strIndent << "for( const Ed::Node& n" << ( iNodeCount + 1 ) << " : n" << iNodeCount << ".children )\n";
    os << strIndent << "{\n";
    os << strIndent << "  const Ed::Identifier& identity = n" << ( iNodeCount + 1 ) << ".statement.declarator.identifier.get();\n";
    os << strIndent << "  if( identity == \"\" )\n";
    os << strIndent << "  {\n";
    os << strIndent << "    ERR( \"Node missing identifier\" );\n";
    os << strIndent << "  }\n";
        
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            os << strIndent << "  else if( identity == \"" << pNestedAction->getContext()->getIdentifier() << "\" )\n";
            os << strIndent << "  {\n";
            recurseLoadTree( os, printerFactory, layout, pNestedAction, iNodeCount + 1, strIndent );
            os << strIndent << "  }\n";
        }
        else if( const eg::concrete::Dimension_User* pDimension =
            dynamic_cast< const eg::concrete::Dimension_User* >( pElement ) )
        {
            os << strIndent << "  else if( identity == \"" << pDimension->getDimension()->getIdentifier() << "\" )\n";
            os << strIndent << "  {\n";
            const eg::DataMember* pDataMember = layout.getDataMember( pDimension ); 
            os << strIndent << "    Ed::IShorthandStream is( n" << ( iNodeCount + 1 ) << ".statement.shorthand.get() );\n";
            os << strIndent << "    is >> " << *printerFactory.write( pDataMember, "0" ) << ";\n";
            os << strIndent << "  }\n";
        }
    }
    
    os << strIndent << "  else\n";
    os << strIndent << "  {\n";
    os << strIndent << "    ERR( \"Failed to match node: \" << identity );\n";
    os << strIndent << "  }\n";
    os << strIndent << "}\n";
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
}

void recurseSaveTree( std::ostream& os,
        eg::PrinterFactory& printerFactory, 
        const eg::Layout& layout, 
        const eg::concrete::Action* pAction, 
        int iNodeCount, 
        std::string& strIndent )
{
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' );
    
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            os << strIndent << "{\n";
            os << strIndent << "  Ed::Node n" << ( iNodeCount + 1 ) << ";\n";
            os << strIndent << "  n" << ( iNodeCount + 1 ) << ".statement.declarator.identifier = \"" << 
                pNestedAction->getContext()->getIdentifier() << "\";\n";
            recurseSaveTree( os, printerFactory, layout, pNestedAction, iNodeCount + 1, strIndent );
            os << strIndent << "  n" << iNodeCount << ".children.push_back( n" << ( iNodeCount + 1 ) << " );\n";
            os << strIndent << "}\n";
        }
        else if( const eg::concrete::Dimension_User* pDimension =
            dynamic_cast< const eg::concrete::Dimension_User* >( pElement ) )
        {
            os << strIndent << "{\n";
            os << strIndent << "  Ed::Node n" << ( iNodeCount + 1 ) << ";\n";
            os << strIndent << "  n" << ( iNodeCount + 1 ) << 
                ".statement.declarator.identifier = \"" << 
                pDimension->getDimension()->getIdentifier() << "\";\n";
            const eg::DataMember* pDataMember = layout.getDataMember( pDimension );            
            os << strIndent << "  if( !n" << ( iNodeCount + 1 ) << ".statement.shorthand )\n";
            os << strIndent << "    n" << ( iNodeCount + 1 ) << ".statement.shorthand = Ed::Shorthand();\n";
            os << strIndent << "  Ed::OShorthandStream os( n" << ( iNodeCount + 1 ) << ".statement.shorthand.get() );\n";
            os << strIndent << "  os << " << *printerFactory.read( pDataMember, "0" ) << ";\n";
            os << strIndent << "  n" << iNodeCount << ".children.push_back( n" << ( iNodeCount + 1 ) << " );\n";
            os << strIndent << "}\n";
        }
    }
    
    strIndent.pop_back();
    strIndent.pop_back();
}

void findConfig( const eg::concrete::Action* pAction, std::vector< const eg::concrete::Action* >& results )
{
    if( pAction->getContext()->getIdentifier() == "config" )
    {
        results.push_back( pAction );
    }
    
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            findConfig( pNestedAction, results );
        }
    }
}

void recursePathSearch( std::ostream& os,
        const eg::concrete::Action* pAction, 
        std::string& strIndent, int iIterIndex )
{
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' );
    strIndent.push_back( ' ' ); 
    strIndent.push_back( ' ' );
    
    os << strIndent << "if( boost::apply_visitor( visitor, *i" << iIterIndex << " ) == \"" << pAction->getContext()->getIdentifier() << "\" )\n";
    os << strIndent << "{\n";
    os << strIndent << "  Ed::Reference::const_iterator i" << ( iIterIndex + 1 ) << " = i" << iIterIndex << " + 1;\n";
    os << strIndent << "  if( i" << ( iIterIndex + 1 ) << " == iterEnd )\n";
    os << strIndent << "  {\n";
    os << strIndent << "    return Event( eg::reference{ 0U, " << pAction->getIndex() << ", 0U } );\n";
    os << strIndent << "  }\n";
        
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    bool bFoundNestedAction = false;
    {        
        for( const eg::concrete::Element* pElement : children )
        {
            if( const eg::concrete::Action* pNestedAction = 
                dynamic_cast< const eg::concrete::Action* >( pElement ) )
            {
                bFoundNestedAction = true;
                break;
            }
        }
    }
    if( bFoundNestedAction )
    {
        os << strIndent << "  else\n";
        os << strIndent << "  {\n";
        
        for( const eg::concrete::Element* pElement : children )
        {
            if( const eg::concrete::Action* pNestedAction = 
                dynamic_cast< const eg::concrete::Action* >( pElement ) )
            {
                recursePathSearch( os, pNestedAction, strIndent, iIterIndex + 1 );
            }
        }
        
        os << strIndent << "  }\n";
    }
    
    os << strIndent << "}\n";
    
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
    strIndent.pop_back();
}

void generateGeometryCode( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, 
        eg::PrinterFactory::Ptr pPrinterFactory )
{
    
    os << "#include \"" << projectTree.getGeometryInclude() << "\"\n";
    os << "#include \"geometry/config_conversions.hpp\"\n";
    os << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
    os << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
    os << "#include \"ed/file.hpp\"\n";
    os << "#include <fstream>\n";
    
    const eg::concrete::Action*     pConcreteRoot       = session.getInstanceRoot();
    const eg::Layout&               layout              = session.getLayout();
    const eg::LinkAnalysis&         linkAnaysis         = session.getLinkAnalysis();
    const eg::DerivationAnalysis&   derivationAnalysis  = session.getDerivationAnalysis();
    
    std::vector< const eg::concrete::Action* > results;
    findConfig( pConcreteRoot, results );
    VERIFY_RTE_MSG( !results.empty(), "Failed to find config context" );
    VERIFY_RTE_MSG( results.size() == 1U, "Found multiple config contexts" );
    //find the config node
    
    const eg::concrete::Action* pConfig = results.front();
    
    os << "\n\n";
    os << "\n\n";
        
    os << "void load_config( const std::string& filepath )\n";
    os << "{\n";
    os << "  Ed::Node node;\n";
    os << "  {\n";
    os << "      Ed::BasicFileSystem filesystem;\n";
    os << "      Ed::File edFile( filesystem, filepath );\n";
    os << "      edFile.expandShorthand();\n";
    os << "      edFile.removeTypes();\n";
    os << "      edFile.toNode( node );\n";
    os << "  }\n";
    os << "  if( !node.children.empty() )\n";
    os << "  {\n";
    os << "    Ed::Node& n1 = node.children.front();\n";
    {
        int iNodeCount = 1;
        std::string strIndent = "";
        recurseLoadTree( os, *pPrinterFactory, layout, pConfig, iNodeCount, strIndent );
    }
    os << "  }\n";
    os << "  else\n";
    os << "  {\n";
    os << "    ERR( \"Invalid config file: \" + filepath );\n";
    os << "  }\n";
    os << "}\n";
    os << "void save_config( const std::string& filepath )\n";
    os << "{\n";
    os << "    Ed::Node n1( Ed::Statement( Ed::Declarator( \"config\" ) ) );\n";
    {
        int iNodeCount = 1;
        std::string strIndent = "  ";
        recurseSaveTree( os, *pPrinterFactory, layout, pConfig, iNodeCount, strIndent );
    }
    os << "    std::ofstream os( filepath );\n";
    os << "    os << n1;\n";
    os << "}\n";
    
    os << "struct ReferenceVisitor : boost::static_visitor< const Ed::Identifier& >\n";
    os << "{\n";
    os << "    ReferenceVisitor()\n";
    os << "    {}\n";
    os << "    const Ed::Identifier& operator()( const Ed::RefActionType& ) const\n";
    os << "    {\n";
    os << "        THROW_RTE( \"Reference contains Ed::RefActionType\" );\n";
    os << "    }\n";
    os << "    const Ed::Identifier& operator()( const Ed::Ref& ) const\n";
    os << "    {\n";
    os << "        THROW_RTE( \"Reference contains Ed::Ref\" );\n";
    os << "    }\n";
    os << "    const Ed::Identifier& operator()( const Ed::Identifier& id ) const\n";
    os << "    {\n";
    os << "        return id;\n";
    os << "    }\n";
    os << "};\n";
    
    os << "eg::Event convert_string_to_config_reference( const std::string& strReference )\n";
    os << "{\n";
    os << "  std::ostringstream osError;\n";
    os << "  Ed::Reference ref;\n";
    os << "  const Ed::ParseResult parseResult = Ed::parse( strReference, ref, osError );\n";
    os << "  VERIFY_RTE_MSG( parseResult.first, \"Failed to parse config reference: \" << strReference << \" \" << osError.str() );\n";
    os << "  VERIFY_RTE_MSG( parseResult.second.base() == strReference.end(), \"Failed to parse config reference: \" << strReference );\n";
    os << "  Ed::Reference::const_iterator i1 = ref.begin(), iterEnd = ref.end();\n";
    os << "  ReferenceVisitor visitor;\n";
    {
        std::string strIndent = "";
        recursePathSearch( os, pConfig, strIndent, 1 );
    }
    os << "  eg::Event ev;\n";
    os << "  return ev;\n";
    os << "}\n";
    
    os << "\n";
    
}
