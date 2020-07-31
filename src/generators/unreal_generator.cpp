
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

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/filesystem.hpp>

namespace megastructure
{
extern void generateRuntimeExterns( std::ostream& os, const eg::ReadSession& session );
}

namespace eg
{
    
    inline std::string interfaceName( const interface::Context* pNode )
    {
        std::ostringstream os;
        os << "I" << pNode->getIdentifier();
        return os.str();
    }
    
    void contextInclusion( const interface::Context* pContext, bool& bIsContext, bool& bIsObject )
    {
        bIsContext = false;
        bIsObject = false;
        
        if( const interface::Event* pEvent = dynamic_cast< const interface::Event* >( pContext ) )
        {
        }
        else if( const interface::Function* pFunction = dynamic_cast< const interface::Function* >( pContext ) )
        {
        }
        else if( const interface::Action* pAction = dynamic_cast< const interface::Action* >( pContext ) )
        {
            bIsContext = true;
        }
        else if( const interface::Object* pObject = dynamic_cast< const interface::Object* >( pContext ) )
        {
            bIsContext = true;
            bIsObject = true;
        }
        else if( const interface::Link* pLink = dynamic_cast< const interface::Link* >( pContext ) )
        {
            bIsContext = true;
        }
        else if( const interface::Abstract* pAbstract = dynamic_cast< const interface::Abstract* >( pContext ) )
        {
            bIsContext = true;
        }
        else
        {
            THROW_RTE( "Unknown context type" );
        }
    }

    struct InterfaceVisitor
    {
        std::ostream& os;
        std::string strIndent;

        InterfaceVisitor( std::ostream& os ) : os( os ) {}
        
        void pushIndent()
        {
            strIndent.push_back( ' ' );
            strIndent.push_back( ' ' );
        }
        void popIndent()
        {
            strIndent.pop_back();
            strIndent.pop_back();
        }

        void push ( const input::Opaque*    pElement, const interface::Element* pNode )
        {
        }
        void push ( const input::Dimension* pElement, const interface::Element* pNode )
        {
            const interface::Dimension* pDimension = dynamic_cast< const interface::Dimension* >( pNode );
            
            os << strIndent << "virtual const " << pDimension->getCanonicalType() << "& " << pDimension->getIdentifier() << "() const = 0;\n";
        }
        void push ( const input::Include*   pElement, const interface::Element* pNode )
        {
        }
        void push ( const input::Using*   pElement, const interface::Element* pNode )
        {
        }
        void push ( const input::Export*   pElement, const interface::Element* pNode )
        {
        }
        void push ( const input::Root* pElement, const interface::Element* pNode )
        {
            push( (const input::Context*) pElement, pNode );
        }
        void push ( const input::Context* pElement, const interface::Element* pNode )
        {
            bool bIsContext = false;
            bool bIsObject = false;
            
            const interface::Context* pContext = dynamic_cast< const interface::Context* >( pNode );
            VERIFY_RTE( pContext );
            
            contextInclusion( pContext, bIsContext, bIsObject );
            
            if( bIsContext )
            {
                const std::vector< interface::Context* >& baseContexts = pContext->getBaseContexts();
                if( !baseContexts.empty() )
                {
                    std::ostringstream osBaseList;
                    bool bFirst = true;
                    for( const interface::Context* pBaseContext: baseContexts )
                    {
                        if( bFirst )
                        {
                            osBaseList << interfaceName( pBaseContext ) << " ";
                            bFirst = false;
                        }
                        else
                        {
                            osBaseList << ", " << interfaceName( pBaseContext ) << " ";
                        }
                    }
                    os << strIndent << "struct " << interfaceName( pContext ) << " : public " << osBaseList.str() << "\n";
                    os << strIndent << "{\n";
                }
                else if( bIsObject )
                {
                    os << strIndent << "struct " << interfaceName( pContext ) << " : public IObject\n";
                    os << strIndent << "{\n";
                }
                else
                {
                    os << strIndent << "struct " << interfaceName( pContext ) << " : public IContext\n";
                    os << strIndent << "{\n";
                }
            }
                
            pushIndent();
        }
        void pop ( const input::Opaque* pElement, const interface::Element* pNode )
        {
        }
        void pop ( const input::Dimension* pElement, const interface::Element* pNode )
        {
        }
        void pop ( const input::Include* pElement, const interface::Element* pNode )
        {
        }
        void pop ( const input::Using* pElement, const interface::Element* pNode )
        {
            //os << strIndent << "using " << pElement->getIdentifier() << " = " << pElement->getType()->getStr() << ";\n";
        }
        void pop ( const input::Export* pElement, const interface::Element* pNode )
        {
        }
        void pop ( const input::Root* pElement, const interface::Element* pNode )
        {
            pop( (const input::Context*) pElement, pNode );
        }
        void pop ( const input::Context* pElement, const interface::Element* pNode )
        {
            popIndent();
            
            bool bIsContext = false;
            bool bIsObject = false;
            
            const interface::Context* pContext = dynamic_cast< const interface::Context* >( pNode );
            VERIFY_RTE( pContext );
            
            contextInclusion( pContext, bIsContext, bIsObject );
            
            if( bIsContext )
            {
                os << strIndent << "};\n";
                if( pContext->isExecutable() )
                {
                    //os << strIndent << "virtual " << interfaceName( pContext ) << "* " << pNode->getIdentifier() << "( std::size_t szIndex ) const = 0;\n";
                    //os << strIndent << "virtual std::size_t " << pNode->getIdentifier() << "_size() const = 0;\n";
                }
            }
        }
    };
}

void generateUnrealInterface( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, eg::PrinterFactory::Ptr pPrinterFactory )
{
    os << "#ifndef UNREAL_INTERFACE\n";
    os << "#define UNREAL_INTERFACE\n";
    
    os << "\n\n";
    
    const eg::interface::Root* pInterfaceRoot = session.getTreeRoot();
    
    os << "struct IObject;\n";
    os << "struct IContext\n";
    os << "{\n";
    //os << "    virtual IObject* getObject() = 0;\n";
    //os << "    virtual IContext* getParent() = 0;\n";
    os << "};\n";
    
    os << "struct IObject : public IContext\n";
    os << "{\n";
    os << "};\n";
    
    {
        os << "\n//Object Interface\n";
        eg::InterfaceVisitor interfaceVisitor( os );
        pInterfaceRoot->pushpop( interfaceVisitor );
        os << "\n";
    }
    
    
    os << "#endif //UNREAL_INTERFACE\n";
}


inline std::string fullInterfaceType( const eg::interface::Context* pNode )
{
    std::vector< const eg::interface::Element* > path =
        eg::interface::getPath( pNode );
        
    std::ostringstream os;
    
    bool bFirst = true;
    for( const eg::interface::Element* pElement : path )
    {
        const eg::interface::Context* pContext = 
            dynamic_cast< const eg::interface::Context* >( pElement );
        if( bFirst )
        {
            os << interfaceName( pContext );
            bFirst = false;
        }
        else
        {
            os << "::" << interfaceName( pContext );
        }
    }
    
    return os.str();
}

inline std::string implName( const eg::concrete::Action* pNode )
{
    std::ostringstream os;
    os << "C" << pNode->getName();
    return os.str();
}

void generateImplementation( std::ostream& os, const eg::Layout& layout, const eg::concrete::Action* pContext, eg::PrinterFactory::Ptr pPrinterFactory )
{
    const std::vector< eg::concrete::Element* >& children = pContext->getChildren();
    
    if( pContext->getParent() )
    {
        os << "struct " << implName( pContext ) << " : public " << fullInterfaceType( pContext->getContext() ) << "\n";
        os << "{\n";
        os << "  " << implName( pContext ) << "( const eg::reference& egRef ) : ref( egRef ) {}\n";
        os << "  eg::reference ref;\n";
        
        for( eg::concrete::Element* pChildElement : children )
        {
            if( const eg::concrete::Dimension_User* pUserDimension = dynamic_cast< const eg::concrete::Dimension_User* >( pChildElement ) )
            {
                const eg::interface::Dimension* pInterfaceDimension = pUserDimension->getDimension();
                os << "  const " << pInterfaceDimension->getCanonicalType() << "& " << 
                    pInterfaceDimension->getIdentifier() << "() const { return " << 
                        *pPrinterFactory->read( layout.getDataMember( pUserDimension ), "ref.instance" ) << "; }\n";
            }
            else if( const eg::concrete::Action* pChildContext = dynamic_cast< const eg::concrete::Action* >( pChildElement ) )
            {
                
            }
        }
        
        os << "};\n";
    
    }
    
    for( const eg::concrete::Element* pChildElement : children )
    {
        if( const eg::concrete::Dimension_User* pUserDimension = dynamic_cast< const eg::concrete::Dimension_User* >( pChildElement ) )
        {
            
            
        }
        else if( const eg::concrete::Action* pChildContext = dynamic_cast< const eg::concrete::Action* >( pChildElement ) )
        {
            bool bIsContext = false;
            bool bIsObject = false;
            
            const eg::interface::Context* pContext = 
                dynamic_cast< const eg::interface::Context* >( pChildContext->getContext() );
            VERIFY_RTE( pContext );
            
            contextInclusion( pChildContext->getContext(), bIsContext, bIsObject );
            
            if( bIsContext )
            {
                generateImplementation( os, layout, pChildContext, pPrinterFactory );
            }
        }
    }
}


void generateAllocations( std::ostream& os, const eg::concrete::Action* pContext )
{
    if( pContext->getParent() )
    {
        std::ostringstream osInit;
        const auto iTotal = pContext->getTotalDomainSize();
        bool bFirst = true;
        for( int i = 0; i < iTotal; ++i )
        {
            if( bFirst )
            {
                osInit << "R{ " << i << ", " << pContext->getIndex() << ", 0 }";
                bFirst = false;
            }
            else
            {
                osInit << ",R{ " << i << ", " << pContext->getIndex() << ", 0 }";
            }
        }
        os << "const std::array< " << implName( pContext ) << ", " << iTotal << " > g_" << implName( pContext ) << " = { " << osInit.str() << " };\n";
    }
    
    const std::vector< eg::concrete::Element* >& children = pContext->getChildren();
    for( const eg::concrete::Element* pChildElement : children )
    {
        if( const eg::concrete::Action* pChildContext = dynamic_cast< const eg::concrete::Action* >( pChildElement ) )
        {
            bool bIsContext = false;
            bool bIsObject = false;
            
            const eg::interface::Context* pContext = 
                dynamic_cast< const eg::interface::Context* >( pChildContext->getContext() );
            VERIFY_RTE( pContext );
            
            contextInclusion( pChildContext->getContext(), bIsContext, bIsObject );
            
            if( bIsContext )
            {
                generateAllocations( os, pChildContext );
            }
        }
    }
}

void generateUnrealCode( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, eg::PrinterFactory::Ptr pPrinterFactory )
{

    os << "#include \"unreal/unreal.hpp\"\n";
    
    os << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
    os << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
    os << "#include \"" << projectTree.getUnrealInterfaceInclude() << "\"\n\n";
    
    const eg::concrete::Action* pConcreteRoot = session.getInstanceRoot();
    const eg::Layout& layout = session.getLayout();
    
    generateImplementation( os, layout, pConcreteRoot, pPrinterFactory );
    
    os << "\n";
    os << "using R = eg::reference;\n";
    
    generateAllocations( os, pConcreteRoot );
    
    os << "\n";

    os << "void* getUnrealRoot()\n";
    os << "{\n";
    os << "    return &g_Croot_100[ 0 ];\n";
    os << "}\n";
    
    //megastructure::generateRuntimeExterns( os, session );
}
