
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

void generateLoad( const eg::concrete::Action* pAction )
{
    
    bool bFoundConstDimension = false;
    
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Dimension_User* pUserDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( pElement ) )
        {
            if( pUserDimension->getDimension()->isConst() )
            {
                bFoundConstDimension = true;
            }
        }
    }
    
    
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            generateLoad( pNestedAction );
        }
    }
}

void generateConfigIO( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, 
        eg::PrinterFactory::Ptr pPrinterFactory )
{
    
    const eg::concrete::Action*     pConcreteRoot       = session.getInstanceRoot();
    const eg::Layout&               layout              = session.getLayout();
    const eg::LinkAnalysis&         linkAnaysis         = session.getLinkAnalysis();
    const eg::DerivationAnalysis&   derivationAnalysis  = session.getDerivationAnalysis();
    
    generateLoad( pConcreteRoot );
    
    
    os << "#include \"geometry/config_conversions.hpp\"\n";
    os << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
    os << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
    os << "#include \"ed/file.hpp\"\n";
    os << "#include <fstream>\n";
    
    os << "void config_io_load( const char* pszSrcFolderPath )\n";
    os << "{\n";
    os << "  LOG( \"config_io_load: \" << pszSrcFolderPath );\n";
    //os << "  Ed::Node node;\n";
    //os << "  {\n";
    //os << "      Ed::BasicFileSystem filesystem;\n";
    //os << "      Ed::File edFile( filesystem, pszFilePath );\n";
    //os << "      edFile.expandShorthand();\n";
    //os << "      edFile.removeTypes();\n";
    //os << "      edFile.toNode( node );\n";
    //os << "  }\n";
    os << "}\n";
    os << "void config_io_save( const char* pszSrcFolderPath )\n";
    os << "{\n";
    os << "  LOG( \"config_io_save: \" << pszSrcFolderPath );\n";
    //os << "  Ed::Node n1( Ed::Statement( Ed::Declarator( \"config\" ) ) );\n";
    //os << "  std::ofstream os( pszFilePath );\n";
    //os << "  os << n1;\n";
    os << "}\n";
}