
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
    
    bool dataTypeSupported( const interface::Dimension* pDimension )
    {
        if( pDimension->getContextTypes().empty() )
        {
            return true;
        }
        else if( pDimension->getContextTypes().size() == 1U )
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    std::string dataType( const interface::Dimension* pDimension )
    {
        std::ostringstream os;
        if( pDimension->getContextTypes().empty() )
        {
            os << "const " << pDimension->getCanonicalType() << "&";
        }
        else if( pDimension->getContextTypes().size() == 1U )
        {
            const interface::Context* pAction = pDimension->getContextTypes().front();
            
            os << "const " << interfaceName( pAction ) << "*";
        }
        else
        {
            THROW_RTE( "Variants not supported in interface" );
            /*os << EG_VARIANT_TYPE << "< ";
            for( const interface::Context* pAction : pDimension->getContextTypes() )
            {
                if( pAction != pDimension->getContextTypes().front() )
                    os << ", ";
                os << getStaticType( pAction );
            }
            os << " >";*/
        }
        return os.str();
    }

    struct StaticInterfaceVisitor
    {
        const eg::LinkAnalysis& linkAnalysis;
        std::ostream& os;
        std::string strIndent;

        StaticInterfaceVisitor( const eg::LinkAnalysis& _linkAnalysis, std::ostream& os ) : linkAnalysis( _linkAnalysis ), os( os ) {}
        
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
            if( dataTypeSupported( pDimension ) )
            {
                os << strIndent << "virtual " << dataType( pDimension ) << " " << pDimension->getIdentifier() << "() const = 0;\n";
            }
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
        void push ( const input::Visibility* pElement, const interface::Element* pNode )
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
                
                if( const interface::Link* pLink = dynamic_cast< const interface::Link* >( pContext ) )
                {
                    os << strIndent << "struct " << interfaceName( pContext ) << " : public IContext\n";
                    os << strIndent << "{\n";
                }
                else
                {
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
                
                //link groups
                const LinkGroup::Vector& linkGroups = linkAnalysis.getLinkGroups();
                for( const LinkGroup* pLinkGroup : linkGroups )
                {
                    const std::vector< interface::Context* >& targets = pLinkGroup->getInterfaceTargets();
                    if( std::find( targets.begin(), targets.end(), pContext ) != targets.end() )
                    {
                        //generate linkgroup access
                        os << strIndent << "  virtual const IContext* " << pLinkGroup->getLinkName() << "() const = 0;\n";
                    }
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
        void pop ( const input::Visibility* pElement, const interface::Element* pNode )
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
                //if( pContext->isExecutable() )
                if( pContext->getParent() && pContext->getParent()->getParent() )
                {
                    os << strIndent << "virtual const " << interfaceName( pContext ) << "* " << pNode->getIdentifier() << "( std::size_t iterator ) const = 0;\n";
                    os << strIndent << "virtual std::size_t " << pNode->getIdentifier() << "_begin() const = 0;\n";
                    os << strIndent << "virtual std::size_t " << pNode->getIdentifier() << "_next( std::size_t iterator ) const = 0;\n";
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
    os << "    virtual std::size_t getInstance() const = 0;\n";
    //os << "    virtual std::size_t getRTTI() const = 0;\n";
    os << "};\n";
    
    os << "struct IObject : public IContext\n";
    os << "{\n";
    os << "};\n";
    
    
    const eg::LinkAnalysis& linkAnalysis = session.getLinkAnalysis();
    
    {
        os << "\n//Object Interface\n";
        eg::StaticInterfaceVisitor interfaceVisitor( linkAnalysis, os );
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
inline std::string arrayName( const eg::concrete::Action* pNode )
{
    std::ostringstream os;
    os << "u_" << pNode->getName();
    return os.str();
}

std::string dataTypeFull( const eg::interface::Dimension* pDimension )
{
    std::ostringstream os;
    if( pDimension->getContextTypes().empty() )
    {
        os << "const " << pDimension->getCanonicalType() << "&";
    }
    else if( pDimension->getContextTypes().size() == 1U )
    {
        const eg::interface::Context* pAction = pDimension->getContextTypes().front();
        os << "const " << fullInterfaceType( pAction ) << "*";
    }
    else
    {
        THROW_RTE( "Variants not supported in interface" );
    }
    return os.str();
}
    
void generateImplementationDeclarations( std::ostream& os, const eg::LinkAnalysis& linkAnalysis, 
    const eg::Layout& layout, const eg::concrete::Action* pContext, eg::PrinterFactory::Ptr pPrinterFactory )
{
    const std::vector< eg::concrete::Element* >& children = pContext->getChildren();
    
    if( pContext->getParent() )
    {
        os << "struct " << implName( pContext ) << " : public " << fullInterfaceType( pContext->getContext() ) << "\n";
        os << "{\n";
        os << "  " << implName( pContext ) << "( const eg::TypeInstance& egRef ) : ref( egRef ) {}\n";
        os << "  eg::TypeInstance ref;\n";
        os << "  std::size_t getInstance() const { return ref.instance; };\n";
        //os << "  virtual std::size_t getRTTI() const { return ref.type; };\n";
        
        //link groups
        const eg::concrete::Action::LinkMap& links = pContext->getLinks();
        for( eg::concrete::Action::LinkMap::const_iterator i = links.begin(),
            iEnd = links.end(); i!=iEnd; ++i )
        {
            os << "  virtual const IContext* " << i->first << "() const;\n";
        }
        
        for( eg::concrete::Element* pChildElement : children )
        {
            if( const eg::concrete::Dimension_User* pUserDimension = dynamic_cast< const eg::concrete::Dimension_User* >( pChildElement ) )
            {
                if( const eg::LinkGroup* pLinkGroup = pUserDimension->getLinkGroup() )
                {
                    //convert the link reference to the interface type
                    const eg::interface::Dimension* pDimension = pUserDimension->getDimension();
                    os << "  " << dataTypeFull( pDimension ) << " " << pDimension->getIdentifier() << "() const;\n";
                }
                else
                {
                    const eg::interface::Dimension* pDimension = pUserDimension->getDimension();
                    if( dataTypeSupported( pDimension ) )
                    {
                        os << "  " << dataTypeFull( pDimension ) << " " << pDimension->getIdentifier() << "() const;\n";
                    }
                }
            }
            else if( const eg::concrete::Action* pChildContext = dynamic_cast< const eg::concrete::Action* >( pChildElement ) )
            {
                bool bIsContext = false;
                bool bIsObject = false;
                
                contextInclusion( pChildContext->getContext(), bIsContext, bIsObject );
                
                if( bIsContext )
                {
                    if( pContext->getParent() )
                    {
                        os << "  const " << interfaceName( pChildContext->getContext() ) << "* " << pChildContext->getContext()->getIdentifier() << "( std::size_t iterator ) const;\n";
                        os << "  std::size_t " << pChildContext->getContext()->getIdentifier() << "_begin() const;\n";
                        os << "  std::size_t " << pChildContext->getContext()->getIdentifier() << "_next( std::size_t iterator ) const;\n";
                    }
                }
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
                generateImplementationDeclarations( os, linkAnalysis, layout, pChildContext, pPrinterFactory );
            }
        }
    }
}


void generateAllocations( std::ostream& os, const eg::concrete::Action* pContext )
{
    VERIFY_RTE( pContext->getParent() );
    
    std::ostringstream osInit;
    const auto iTotal = pContext->getTotalDomainSize();
    bool bFirst = true;
    for( int i = 0; i < iTotal; ++i )
    {
        if( bFirst )
        {
            osInit << "R{ " << i << ", " << pContext->getIndex() << " }";
            bFirst = false;
        }
        else
        {
            osInit << ",R{ " << i << ", " << pContext->getIndex() << " }";
        }
    }
    os << "const std::array< " << implName( pContext ) << ", " << iTotal << " > " << arrayName( pContext ) << " = { " << osInit.str() << " };\n";
    
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

void generateImplementationDefinitions( std::ostream& os, const eg::LinkAnalysis& linkAnalysis, 
    const eg::Layout& layout, const eg::DerivationAnalysis& derivationAnalysis, 
    const eg::concrete::Action* pContext, eg::PrinterFactory::Ptr pPrinterFactory )
{
    const std::vector< eg::concrete::Element* >& children = pContext->getChildren();
    
    if( pContext->getParent() )
    {
        //link groups
        const eg::concrete::Action::LinkMap& links = pContext->getLinks();
        for( eg::concrete::Action::LinkMap::const_iterator i = links.begin(),
            iEnd = links.end(); i!=iEnd; ++i )
        {
            const eg::concrete::Dimension_Generated* pBackReference = i->second;
            const eg::LinkGroup* pLinkGroup = pBackReference->getLinkGroup();
            VERIFY_RTE( pLinkGroup );
            
            //generate linkgroup access
            os << "const IContext* " << implName( pContext ) << "::" << i->first << "() const\n";
            os << "{\n";
            os << "  const auto& backLinkRef = " << *pPrinterFactory->read( layout.getDataMember( pBackReference ), "ref.instance" ) << ";\n";
            os << "  switch( backLinkRef.type )\n";
            os << "  {\n";
            for( const eg::concrete::Action* pConcreteLinkType : pLinkGroup->getConcreteLinks() )
            {
            os << "    case " << pConcreteLinkType->getIndex() << ": return &" << arrayName( pConcreteLinkType ) << "[ backLinkRef.instance ];\n";
            }
            os << "    default: return nullptr;\n";
            os << "  }\n";
            os << "}\n";
        }
        
        const eg::LinkGroup::Vector& linkGroups = linkAnalysis.getLinkGroups();
        for( const eg::LinkGroup* pLinkGroup : linkGroups )
        {
            const std::vector< eg::concrete::Action* >& targets = pLinkGroup->getTargets();
            if( std::find( targets.begin(), targets.end(), pContext ) != targets.end() )
            {
            }
        }
        
        for( eg::concrete::Element* pChildElement : children )
        {
            if( const eg::concrete::Dimension_User* pUserDimension = dynamic_cast< const eg::concrete::Dimension_User* >( pChildElement ) )
            {
                if( const eg::LinkGroup* pLinkGroup = pUserDimension->getLinkGroup() )
                {
                    //convert the link reference to the interface type
                    const eg::interface::Dimension* pDimension = pUserDimension->getDimension();
                    
                    os << dataTypeFull( pDimension ) << " " << implName( pContext ) << "::" << pDimension->getIdentifier() << "() const\n";
                    os << "{\n";
                    os << "  const auto& linkRef = " << *pPrinterFactory->read( layout.getDataMember( pUserDimension ), "ref.instance" ) << ";\n";
                    os << "  switch( linkRef.data.type )\n";
                    os << "  {\n";
                    
                    const eg::interface::Context* pLinkTargetType = nullptr;
                    if( pDimension->getContextTypes().size() == 1U )
                        pLinkTargetType = pDimension->getContextTypes().front();
                    VERIFY_RTE( pLinkTargetType );
                    
                    const eg::DerivationAnalysis::Compatibility& compatibility =
                        derivationAnalysis.getCompatibility( pLinkTargetType );
                    
                    for( const eg::concrete::Action* pConcreteLinkType : compatibility.dynamicCompatibleTypes )
                    {
                    os << "      case " << pConcreteLinkType->getIndex() << ": return &" << arrayName( pConcreteLinkType ) << "[ linkRef.data.instance ];\n";
                    }
                    os << "      default: return nullptr;\n";
                    os << "  }\n";
                    os << "}\n";
                }
                else
                {
                    const eg::interface::Dimension* pDimension = pUserDimension->getDimension();
                    if( dataTypeSupported( pDimension ) )
                    {
                        os << dataTypeFull( pDimension ) << " " << implName( pContext ) << "::" << pDimension->getIdentifier() << "() const { return " << 
                                *pPrinterFactory->read( layout.getDataMember( pUserDimension ), "ref.instance" ) << "; }\n";
                    }
                }
                
            }
            else if( const eg::concrete::Action* pChildContext = dynamic_cast< const eg::concrete::Action* >( pChildElement ) )
            {
                bool bIsContext = false;
                bool bIsObject = false;
                
                contextInclusion( pChildContext->getContext(), bIsContext, bIsObject );
                
                if( bIsContext )
                {
                    if( pContext->getParent() )
                    {
                        os << "const " << fullInterfaceType( pChildContext->getContext() ) << "* " << 
                                implName( pContext ) << "::" << pChildContext->getContext()->getIdentifier() << "( std::size_t iterator ) const\n";
                        os << "{\n";
                        os << "    const std::size_t _end = ( ref.instance + 1 ) * " << pChildContext->getLocalDomainSize() << ";\n";
                        os << "    return ( iterator != _end ) ? &" << arrayName( pChildContext ) << "[ iterator ] : nullptr;\n";
                        os << "}\n";
                        
                        os << "std::size_t " << implName( pContext ) << "::" << pChildContext->getContext()->getIdentifier() << "_begin() const\n";
                        os << "{\n"; 
                        os << "    const std::size_t _end = ( ref.instance + 1 ) * " << pChildContext->getLocalDomainSize() << ";\n";
                        os << "    std::size_t iterator = ref.instance * " << pChildContext->getLocalDomainSize() << ";\n";
                        os << "    for( ; iterator != _end; ++iterator )\n";
                        os << "    {\n";
                        os << "        if( isActionActive< " << getStaticType( pChildContext->getContext() ) << " >( " << pChildContext->getIndex() << ", iterator )  )\n";
                        os << "            break;\n";
                        os << "    }\n";
                        os << "    return iterator;\n";
                        os << "}\n";
                        
                        os << "std::size_t " << implName( pContext ) << "::" << pChildContext->getContext()->getIdentifier() << "_next( std::size_t iterator ) const\n";
                        os << "{\n";
                        os << "    const std::size_t _end = ( ref.instance + 1 ) * " << pChildContext->getLocalDomainSize() << ";\n";
                        os << "    for( ; iterator != _end;  )\n";
                        os << "    {\n";
                        os << "        ++iterator;\n";
                        os << "        if( isActionActive< " << getStaticType( pChildContext->getContext() ) << " >( " << pChildContext->getIndex() << ", iterator )  )\n";
                        os << "            break;\n";
                        os << "    }\n";
                        os << "    return iterator;\n";
                        os << "}\n";
                        
                    }
                }
            }
        }
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
                generateImplementationDefinitions( os, linkAnalysis, layout, derivationAnalysis, pChildContext, pPrinterFactory );
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
    const eg::LinkAnalysis& linkAnaysis = session.getLinkAnalysis();
    const eg::DerivationAnalysis& derivationAnalysis = session.getDerivationAnalysis();
    
    VERIFY_RTE( !pConcreteRoot->getParent() );
    VERIFY_RTE( pConcreteRoot->getChildren().size() == 1U );
    
    const eg::concrete::Action* pActualRoot = 
        dynamic_cast< const eg::concrete::Action* >( pConcreteRoot->getChildren().front() );
        
    eg::generateMemberFunctions( os, *pPrinterFactory, session );
    
    generateImplementationDeclarations( os, linkAnaysis, layout, pActualRoot, pPrinterFactory );
    
    os << "\n";
    os << "using R = eg::TypeInstance;\n";
    
    generateAllocations( os, pActualRoot );
    
    generateImplementationDefinitions( os, linkAnaysis, layout, derivationAnalysis, pActualRoot, pPrinterFactory );
    
    os << "\n";

    os << "void* getUnrealRoot()\n";
    os << "{\n";
    os << "    return const_cast< void* >( reinterpret_cast< const void* >( &" << arrayName( pActualRoot ) << "[ 0 ] ) );\n";
    os << "}\n";
    
}
