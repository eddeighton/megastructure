
#include <ostream>

#include "egcomponent/generator.hpp"

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

void generateTypeCasters( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree )
{
    const eg::interface::Root* pRoot = session.getTreeRoot();
    const eg::concrete::Action* pInstanceRoot = nullptr;
    {
        std::vector< const eg::concrete::Action* > roots;
        for( const eg::concrete::Element* pChild : session.getInstanceRoot()->getChildren() )
        {
            if( const eg::concrete::Action* pAction = 
                dynamic_cast< const eg::concrete::Action* >( pChild ) )
            {
                roots.push_back( pAction );
            }
        }
        ASSERT( !roots.empty() );
        ASSERT( roots.size() == 1U );
        pInstanceRoot = roots.front();
    }

    const eg::DerivationAnalysis& derivationAnalysis = session.getDerivationAnalysis();
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
    std::vector< const eg::concrete::Action* > actions = 
        eg::many_cst< eg::concrete::Action >( objects );

    os << "std::shared_ptr< megastructure::PythonEGReferenceFactory > g_pEGRefType;\n";
    
    os << "\n";
    os << "//Python Interface Converters\n";
    os << "namespace pybind11\n";
    os << "{\n";
    os << "    namespace detail\n";
    os << "    {\n";

    std::vector< const eg::interface::Context* > abstractActions = 
        eg::many_cst< eg::interface::Context >( objects );
        
    using ActionTypeMap = std::map< const eg::interface::Context*, std::string, eg::CompareIndexedObjects >;
    ActionTypeMap actionTypeMap;

    for( const eg::interface::Context* pAbstractAction : abstractActions )
    {
        if( pAbstractAction->getParent() )
        {
            std::ostringstream osType;
            {
                std::vector< const eg::interface::Element* > path = eg::interface::getPath( pAbstractAction );
                //generate type explicit template specialisation
                {
                    for( const eg::interface::Element* pNodeIter : path )
                    {
                        if( pNodeIter != *path.begin())
                            osType << "::";
                        osType << eg::getInterfaceType( pNodeIter->getIdentifier() ) << "< void >";
                    }
                }
            }
            actionTypeMap.insert( std::make_pair( pAbstractAction, osType.str() ) );
        }
    }
    
    std::string strReferenceTypeName;
    {
        std::ostringstream osRefTypeName;
        osRefTypeName << projectTree.getComponentFileName( true ) << ".reference";
        strReferenceTypeName = osRefTypeName.str();
    }

    os << "        template <> struct type_caster< eg::Event >\n";
    os << "        {\n";
    os << "        public:\n";
    os << "            PYBIND11_TYPE_CASTER( eg::Event, _(\"" << strReferenceTypeName << "\"));\n";
    os << "        \n";
    os << "            bool load( handle src, bool )\n";
    os << "            {\n";
    os << "                const megastructure::PythonEGReference* pEGReference =\n";
    os << "                    megastructure::PythonEGReferenceFactory::getReference( src.ptr() );\n";
    os << "                value.data = pEGReference->getEGReference();\n";
    os << "                return !PyErr_Occurred();\n";
    os << "            }\n";
    os << "        \n";
    os << "            static handle cast( eg::Event src, return_value_policy /* policy */, handle /* parent */)\n";
    os << "            {\n";
    os << "                return g_pEGRefType->create( src.data );\n";
    os << "            }\n";
    os << "        };\n";

    //os << "        template <> struct type_caster< std::vector< eg::Event > >\n";
    //os << "        {\n";
    //os << "        public:\n";
    //os << "            PYBIND11_TYPE_CASTER( std::vector< eg::Event >, _(\"" << strReferenceTypeName << "\"));\n";
    //os << "        \n";
    //os << "            bool load( handle src, bool )\n";
    //os << "            {\n";
    //os << "                //const megastructure::PythonEGReference* pEGReference =\n";
    //os << "                //    megastructure::PythonEGReferenceFactory::getReference( src.ptr() );\n";
    //os << "                //value.data = pEGReference->getEGReference();\n";
    //os << "                //value.data = std::vector< eg::Event >();\n";
    //os << "                return !PyErr_Occurred();\n";
    //os << "            }\n";
    //os << "        \n";
    //os << "            static handle cast( std::vector< eg::Event > src, return_value_policy /* policy */, handle /* parent */)\n";
    //os << "            {\n";
    //os << "                return nullptr;//g_pEGRefType->create( src.data );\n";
    //os << "            }\n";
    //os << "        };\n";

    for( ActionTypeMap::const_iterator 
            i = actionTypeMap.begin(),
            iEnd = actionTypeMap.end(); i!=iEnd; ++i )
    {
        const eg::interface::Context* pAbstractAction = i->first;
        const std::string& strType = i->second;
    os << "        template <> struct type_caster< " << strType << " >\n";
    os << "        {\n";
    os << "        public:\n";
    os << "            PYBIND11_TYPE_CASTER( " << strType << ", _(\"" << strReferenceTypeName << "\"));\n";
    os << "        \n";
    os << "            bool load( handle src, bool )\n";
    os << "            {\n";
    os << "                const megastructure::PythonEGReference* pEGReference =\n";
    os << "                    megastructure::PythonEGReferenceFactory::getReference( src.ptr() );\n";
    os << "                value.data = pEGReference->getEGReference();\n";
    os << "                return !PyErr_Occurred();\n";
    os << "            }\n";
    os << "        \n";
    os << "            static handle cast( " << strType << " src, return_value_policy /* policy */, handle /* parent */)\n";
    os << "            {\n";
    os << "                return g_pEGRefType->create( src.data );\n";
    os << "            }\n";
    os << "        };\n";
    }
    os << "    }   //namespace detail\n";
    os << "} // namespace pybind11\n";

    os << "\n";
}



bool isContextWithinComponent( const ProjectTree& projectTree, 
        const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
        const eg::interface::Context* pContext )
{
    const eg::TranslationUnit* pTU = translationUnitAnalysis.getActionTU( pContext );
    return pTU->getCoordinatorHostnameDefinitionFile().isCoordinator( projectTree.getCoordinatorName() ) &&
        pTU->getCoordinatorHostnameDefinitionFile().isHost( projectTree.getHostName() );
}

void generateRuntimeInterface( std::ostream& os, const eg::ReadSession& session, 
        const Environment& environment, const ProjectTree& projectTree, eg::PrinterFactory::Ptr pPrinterFactory )
{
    
    const char* pszComponentInteropImpl = R"(

struct PythonInterop : public eg::ComponentInterop, public eg::RuntimeTypeInterop
{
private:
    struct Evaluation : public eg::RuntimeTypeInterop::Evaluation
    {
        PyObject *args;
        PyObject *kwargs;
        pybind11::object m_result;
        Evaluation( PyObject *args, PyObject *kwargs )
            :   args( args ), kwargs( kwargs )
        {
            Py_INCREF( args );
        }
        ~Evaluation()
        {
            Py_DECREF( args );
        }
        using WeakPtr = std::weak_ptr< Evaluation >;
        using SharedPtr = std::shared_ptr< Evaluation >;
        
        //eg::RuntimeTypeInterop::Evaluation
        virtual bool isResult() const
        {
            return m_result ? true : false;
        }
        
        virtual void* getResult() const
        {
            pybind11::handle h = m_result;
            h.inc_ref();
            return h.ptr();
        }
    };
public:
    //eg::ComponentInterop
    virtual eg::reference dereferenceDimension( const eg::reference& action, const eg::TypeID& dimensionType );
    virtual void doRead( const eg::reference& reference, eg::TypeID dimensionType );
    virtual void doWrite( const eg::reference& reference, eg::TypeID dimensionType );
    virtual void doCall( const eg::reference& reference, eg::TypeID actionType );
    virtual void doStart( const eg::reference& reference, eg::TypeID actionType );
    virtual void doStop( const eg::reference& reference );
    virtual void doPause( const eg::reference& reference );
    virtual void doResume( const eg::reference& reference );
    virtual void doDone( const eg::reference& reference );
    virtual void doWaitAction( const eg::reference& reference );
    virtual void doWaitDimension( const eg::reference& reference, eg::TypeID dimensionType );
    virtual void doGetAction( const eg::reference& reference );
    virtual void doGetDimension( const eg::reference& reference, eg::TypeID dimensionType );
    virtual void doRange( eg::EGRangeDescriptionPtr pRange );
    virtual void doLink( const eg::reference& linkeeRef, eg::TypeID linkeeDimension, const eg::reference& linkValue );
    
    //eg::RuntimeTypeInterop
    virtual eg::RuntimeTypeInterop::Evaluation::Ptr begin( void* pArgs, void* pKWArgs )
    {
        Evaluation::SharedPtr pEvaluation = std::make_shared< Evaluation >( 
            reinterpret_cast< PyObject* >( pArgs ), 
            reinterpret_cast< PyObject* >( pKWArgs ) );
        m_pEvaluation = pEvaluation;
        return pEvaluation;
    }
    
    virtual eg::TimeStamp   getTimestamp( eg::TypeID type, eg::Instance instance );
    virtual eg::ActionState getState( eg::TypeID type, eg::Instance instance );
    virtual eg::TimeStamp   getStopCycle( eg::TypeID type, eg::Instance instance );
    virtual eg::TimeStamp   getClockCycle( eg::TypeID type );
private:
    Evaluation::WeakPtr m_pEvaluation;
};

static PythonInterop g_interop;

eg::ComponentInterop& getPythonInterop()
{
    return g_interop;
}

)";

    os << pszComponentInteropImpl;
    
    os << "void setEGRuntime( eg::EGRuntime& egRuntime )\n";
    os << "{\n";
    os << "    g_pEGRefType = std::make_shared< megastructure::PythonEGReferenceFactory >( \"" << 
        projectTree.getComponentFileName( true ) << "\", egRuntime, g_interop );\n";
    os << "}\n\n";
    
    
    
    const eg::Layout& layout = session.getLayout();
    const eg::TranslationUnitAnalysis& translationUnitAnalysis = session.getTranslationUnitAnalysis();
    
    
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
    
    std::vector< const eg::concrete::Action* > actions = 
        eg::many_cst< eg::concrete::Action >( objects );
        
    std::vector< const eg::DataMember* > dataMembers;
    {
        for( const eg::Buffer* pBuffer : layout.getBuffers() )
        {
            for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
            {
                dataMembers.push_back( pDataMember );
            }
        }
    }
        
    //std::vector< const eg::DataMember* > dataMembers = 
    //    eg::many_cst< eg::DataMember >( objects );
    
    std::vector< const eg::DataMember* > referenceDimensions;
    for( const eg::DataMember* pDataMember : dataMembers )
    {
        if( const eg::concrete::Dimension_User* pDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( 
                pDataMember->getInstanceDimension() ) )
        {
            if( pDimension->isEGType() )
            {
                referenceDimensions.push_back( pDataMember );
            }
        }
    }
    
    //virtual eg::reference dereferenceDimension( const eg::reference& action, const eg::TypeID& dimensionType );
    os << "" << eg::EG_REFERENCE_TYPE << " PythonInterop::dereferenceDimension( const " << eg::EG_REFERENCE_TYPE << "& action, const " << eg::EG_TYPE_ID << "& dimensionType )\n";
    os << "{\n";
    if( !referenceDimensions.empty() )
    {
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( dimensionType )\n";
    os << "        {\n";
        for( const eg::DataMember* pDataMember : referenceDimensions )
        {
            if( const eg::concrete::Dimension_User* pDimension = 
                dynamic_cast< const eg::concrete::Dimension_User* >( pDataMember->getInstanceDimension() ) )
            {
                if( pDimension->isEGType() )
                {
    os << "            case " << pDimension->getIndex() << ":\n";
    os << "                return " << *pPrinterFactory->read( pDataMember, "action.instance" ) << ".data;\n";
                }
            }
        }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "    return " << eg::EG_REFERENCE_TYPE << "{ 0, 0, 0 };\n";
    }
    else
    {
    os << "    throw std::runtime_error( \"Unreachable\" );\n";
    }
    os << "}\n";
    
    //virtual void doRead( const eg::reference& reference, eg::TypeID dimensionType );
    os << "void PythonInterop::doRead( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " dimensionType )\n";
    os << "{\n";
    
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( dimensionType )\n";
    os << "        {\n";
    for( const eg::DataMember* pDataMember : dataMembers )
    {
        if( const eg::concrete::Dimension_User* pDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( pDataMember->getInstanceDimension() ) )
        {
    os << "            case " << pDimension->getIndex() << ":\n";
    os << "                pEvaluation->m_result = pybind11::cast( " << *pPrinterFactory->read( pDataMember, "reference.instance" ) << " );\n";
    os << "                break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
           
    os << "}\n";
    
    //virtual void doWrite( const eg::reference& reference, eg::TypeID dimensionType );
    os << "void PythonInterop::doWrite( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " dimensionType )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        pybind11::args args = pybind11::reinterpret_borrow< pybind11::args >( pEvaluation->args );\n";
    os << "        switch( dimensionType )\n";
    os << "        {\n";
    
    for( const eg::DataMember* pDataMember : dataMembers )
    {
        if( const eg::concrete::Dimension_User* pDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( pDataMember->getInstanceDimension() ) )
        {
    os << "            case " << pDimension->getIndex() << ":\n";
    os << "                 " << *pPrinterFactory->write( pDataMember, "reference.instance" ) << " = pybind11::cast< "; eg::generateDataMemberType( os, pDataMember ); os << " >( args[ 0 ] );\n";
    os << "                 break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    std::string strIndent = "                    ";
    //virtual void doCall( const eg::reference& reference, eg::TypeID actionType );
    os << "void PythonInterop::doCall( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " actionType )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        pybind11::args args = pybind11::reinterpret_borrow< pybind11::args >( pEvaluation->args );\n";
    os << "        switch( actionType )\n";
    os << "        {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getContext()->isExecutable() && !pAction->getContext()->isMainExecutable() )
        {
			VERIFY_RTE( pAction->getParent() && pAction->getParent()->getParent() );
    os << "            case " << pAction->getIndex() << ":\n";
    os << "                {\n";
    
            
            const eg::concrete::Action* pParent = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
            VERIFY_RTE( pParent );
            const eg::concrete::Allocator* pAllocator = pParent->getAllocator( pAction );
            VERIFY_RTE( pAllocator );
            const eg::interface::Context* pStaticType = pAction->getContext();
            VERIFY_RTE( pStaticType );
            
            bool bVisibilityGuard = false;
            if( pStaticType->getVisibility() == eg::eVisPrivate )
            {
                const std::string strDefine = 
                    translationUnitAnalysis.getActionTU( pStaticType )->getCoordinatorHostnameDefinitionFile().getHostDefine();
                if( !strDefine.empty() )
                {
                    os << "#ifdef " << strDefine << "\n";
                    bVisibilityGuard = true;
                }
            }
            
            const bool bContextIsWithinComponent = 
                isContextWithinComponent( projectTree, translationUnitAnalysis, pStaticType );
                
            if( const eg::interface::Abstract* pContext = dynamic_cast< const eg::interface::Abstract* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempt to call abstract type\" );\n";
            }
            else if( const eg::interface::Event* pContext = dynamic_cast< const eg::interface::Event* >( pStaticType ) )
            {
                const eg::concrete::SingletonAllocator* pSingletonAllocator =
                    dynamic_cast< const eg::concrete::SingletonAllocator* >( pAllocator );
                const eg::concrete::RangeAllocator* pRangeAllocator =
                    dynamic_cast< const eg::concrete::RangeAllocator* >( pAllocator );
                    
                if( const eg::concrete::NothingAllocator* pNothingAllocator =
                    dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) )
                {
                    os << strIndent << eg::getStaticType( pStaticType ) << " ref = " << eg::EG_REFERENCE_TYPE << 
                        "{ reference.instance, " << pAction->getIndex() << ", clock::cycle( " << pAction->getIndex() << " ) };\n";
                    if( bContextIsWithinComponent )
                    {
                    os << strIndent << "::eg::Scheduler::signal_ref( ref );\n";
                    }  
                    else
                    {
                    os << strIndent << "ERR( \"Attempt to call event from wrong component\" );\n";
                    }
                    os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( g_pEGRefType->create( ref.data ) );\n";
                }
                else if( pSingletonAllocator || pRangeAllocator )
                {
                    os << strIndent << eg::getStaticType( pStaticType ) << " ref = " << pAction->getName() << "_starter( reference.instance );\n";
                    os << strIndent << "if( ref )\n";
                    os << strIndent << "{\n";
                    
                    if( bContextIsWithinComponent )
                    {
                    os << strIndent << "    ::eg::Scheduler::signal_ref( ref );\n";
                    }
                    else
                    {
                    os << strIndent << "    ERR( \"Attempt to call event from wrong component\" );\n";
                    }
                    os << strIndent << "    " << pAction->getName() << "_stopper( ref.data.instance );\n";
                    os << strIndent << "}\n";
                    os << strIndent << "else\n";
                    os << strIndent << "{\n";
                    os << strIndent << "    ERR( \"Failed to allocate action: " << pAction->getName() << "\" );\n";
                    os << strIndent << "}\n";
                    os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( g_pEGRefType->create( ref.data ) );\n";
                }
                else
                {
                    THROW_RTE( "Unknown allocator type" );
                }
            }
            else if( const eg::interface::Function* pContext = dynamic_cast< const eg::interface::Function* >( pStaticType ) )
            {
                VERIFY_RTE( dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) );
                //directly invoke the function
                os << strIndent << eg::getStaticType( pStaticType ) << " ref = " << 
                    eg::EG_REFERENCE_TYPE << "{ reference.instance, " << pAction->getIndex() << ", clock::cycle( " << pAction->getIndex() << " ) };\n";
                
                const std::vector< std::string >& parameters = pAction->getContext()->getParameters();
                
                if( pContext->getReturnType() == "void" )
                {
                    os << strIndent << "ref(";
                    bool bFirst = true;
                    int iIndex = 0;
                    for( const std::string& strParamType : parameters )
                    {
                        if( bFirst ) 
                        {
                            bFirst = false;
                            os << " ";
                        }
                        else os << ", ";
                        os << "pybind11::cast< " << strParamType << " >( args[ " << iIndex++ << " ] )";
                    }
                    os << ");\n";
                    os << strIndent << "Py_INCREF( Py_None );\n";
                    os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( Py_None );\n";
                }
                else
                {
                    os << strIndent << "pEvaluation->m_result = pybind11::cast< " << pContext->getReturnType() << " >( ref(";
                    bool bFirst = true;
                    int iIndex = 0;
                    for( const std::string& strParamType : parameters )
                    {
                        if( bFirst ) 
                        {
                            bFirst = false;
                            os << " ";
                        }
                        else os << ", ";
                        os << "pybind11::cast< " << strParamType << " >( args[ " << iIndex++ << " ] )";
                    }
                    os << ") );\n";
                }
            }
            else
            {
                const eg::interface::Action*    pActionContext  = dynamic_cast< const eg::interface::Action* >( pStaticType );
                const eg::interface::Object*    pObjectContext  = dynamic_cast< const eg::interface::Object* >( pStaticType );
                const eg::interface::Link*      pLinkContext    = dynamic_cast< const eg::interface::Link* >( pStaticType );
                
                if( pActionContext || pObjectContext || pLinkContext )
                {
                    os << strIndent << eg::getStaticType( pStaticType ) << " ref = " << pAction->getName() << "_starter( reference.instance );\n";
                        
                    if( pStaticType->hasDefinition() )
                    {
                        os << strIndent << "if( ref )\n";
                        os << strIndent << "{\n";
                        if( bContextIsWithinComponent )
                        {
                        os << strIndent << "    ::eg::Scheduler::call( ref, &" << pAction->getName() << "_stopper );\n";
                        }
                        os << strIndent << "}\n";
                        os << strIndent << "else\n";
                        os << strIndent << "{\n";
                        os << strIndent << "    ERR( \"Failed to allocate action: " << pAction->getName() << "\" );\n";
                        os << strIndent << "}\n";
                        os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( g_pEGRefType->create( ref.data ) );\n";
                    }
                    else
                    {
                        os << strIndent << "if( ref )\n";
                        os << strIndent << "{\n";
                        if( bContextIsWithinComponent )
                        {
                        os << strIndent << "    ::eg::Scheduler::allocated_ref( ref.data, &" << pAction->getName() << "_stopper );\n";
                        }
                        os << strIndent << "}\n";
                        os << strIndent << "else\n";
                        os << strIndent << "{\n";
                        os << strIndent << "    ERR( \"Failed to allocate action: " << pAction->getName() << "\" );\n";
                        os << strIndent << "}\n";
                        os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( g_pEGRefType->create( ref.data ) );\n";
                    }
                }
                else
                {
                    THROW_RTE( "Unknown abstract type" );
                }
            }
            
            if( bVisibilityGuard )
            {
                os << "#else\n";
                os << strIndent << "ERR( \"Attempt to call private context\" );\n";
                os << strIndent << "Py_INCREF( Py_None );\n";
                os << strIndent << "pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( Py_None );\n";
                os << "#endif\n";
            }
            
    os << "                }\n";
    os << "                break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    
    
    //virtual void doStart( const eg::reference& reference, eg::TypeID actionType );
    os << "void PythonInterop::doStart( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " actionType )\n";
    os << "{\n";
    os << "    ERR( \"Start used\" );\n";
    os << "}\n";
    
    //virtual void doStop( const eg::reference& reference );
    os << "void PythonInterop::doStop( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( reference.type )\n";
    os << "        {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getContext()->isExecutable() && !pAction->getContext()->isMainExecutable() )
        {
			VERIFY_RTE( pAction->getParent() && pAction->getParent()->getParent() );
    os << "            case " << pAction->getIndex() << ":\n";
    os << "                {\n";
            const eg::concrete::Action* pParent = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
            VERIFY_RTE( pParent );
            const eg::concrete::Allocator* pAllocator = pParent->getAllocator( pAction );
            VERIFY_RTE( pAllocator );
            const eg::interface::Context* pStaticType = pAction->getContext();
            VERIFY_RTE( pStaticType );
            
            const bool bContextIsWithinComponent = 
                isContextWithinComponent( projectTree, translationUnitAnalysis, pStaticType );
                
            if( const eg::interface::Abstract* pContext = dynamic_cast< const eg::interface::Abstract* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to stop abstract: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Event* pContext = dynamic_cast< const eg::interface::Event* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to stop event: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Function* pContext = dynamic_cast< const eg::interface::Function* >( pStaticType ) )
            {
                VERIFY_RTE( dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) );
                os << strIndent << "ERR( \"Attempted to stop function: " << pAction->getName() << "\" );\n";
            }
            else
            {
                const eg::interface::Action*    pActionContext  = dynamic_cast< const eg::interface::Action* >( pStaticType );
                const eg::interface::Object*    pObjectContext  = dynamic_cast< const eg::interface::Object* >( pStaticType );
                const eg::interface::Link*      pLinkContext    = dynamic_cast< const eg::interface::Link* >( pStaticType );
                
                if( pActionContext || pObjectContext || pLinkContext )
                {
                    if( bContextIsWithinComponent )
                    {
                        os << strIndent << "::eg::Scheduler::stop_ref( reference );\n";
                    }
                    else
                    {
                        os << strIndent << pAction->getName() << "_stopper( reference.instance );\n";
                    }
                }
                else
                {
                    THROW_RTE( "Unknown abstract type" );
                }
            }
    os << "                }\n";
    os << "                break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doPause( const eg::reference& reference );
    os << "void PythonInterop::doPause( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( reference.type )\n";
    os << "        {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getContext()->isExecutable() && !pAction->getContext()->isMainExecutable() )
        {
			VERIFY_RTE( pAction->getParent() && pAction->getParent()->getParent() );
    os << "            case " << pAction->getIndex() << ":\n";
    os << "                {\n";
            const eg::concrete::Action* pParent = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
            VERIFY_RTE( pParent );
            const eg::concrete::Allocator* pAllocator = pParent->getAllocator( pAction );
            VERIFY_RTE( pAllocator );
            const eg::interface::Context* pStaticType = pAction->getContext();
            VERIFY_RTE( pStaticType );
            
            const bool bContextIsWithinComponent = 
                isContextWithinComponent( projectTree, translationUnitAnalysis, pStaticType );
                
            if( const eg::interface::Abstract* pContext = dynamic_cast< const eg::interface::Abstract* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to pause abstract: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Event* pContext = dynamic_cast< const eg::interface::Event* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to pause event: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Function* pContext = dynamic_cast< const eg::interface::Function* >( pStaticType ) )
            {
                VERIFY_RTE( dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) );
                os << strIndent << "ERR( \"Attempted to pause function: " << pAction->getName() << "\" );\n";
            }
            else
            {
                const eg::interface::Action*    pActionContext  = dynamic_cast< const eg::interface::Action* >( pStaticType );
                const eg::interface::Object*    pObjectContext  = dynamic_cast< const eg::interface::Object* >( pStaticType );
                const eg::interface::Link*      pLinkContext    = dynamic_cast< const eg::interface::Link* >( pStaticType );
                
                if( pActionContext || pObjectContext || pLinkContext )
                {
                    const eg::concrete::Dimension_Generated* pStateDim = pAction->getState();
                    VERIFY_RTE( pStateDim );
                    
                    const eg::DataMember* pStateDataMember = layout.getDataMember( pStateDim );
                    VERIFY_RTE( pStateDataMember );
                    
                    os << strIndent << "if( " << *pPrinterFactory->read( pStateDataMember, "reference.instance" ) << 
                        " == " << eg::getActionState( eg::action_running ) << " )\n";
                    os << strIndent << "{\n";
                    
                    if( bContextIsWithinComponent )
                    {
                        os << strIndent << "    ::eg::Scheduler::pause_ref( reference );\n";
                    }
                    os << strIndent << "    " << *pPrinterFactory->write( pStateDataMember, "reference.instance" ) << 
                        " = " << eg::getActionState( eg::action_paused ) << ";\n";
                    os << strIndent << "}\n";
                }
                else
                {
                    THROW_RTE( "Unknown abstract type" );
                }
            }
    os << "                }\n";
    os << "                break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doResume( const eg::reference& reference );
    os << "void PythonInterop::doResume( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( reference.type )\n";
    os << "        {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getContext()->isExecutable() && !pAction->getContext()->isMainExecutable() )
        {
			VERIFY_RTE( pAction->getParent() && pAction->getParent()->getParent() );
    os << "            case " << pAction->getIndex() << ":\n";
    os << "                {\n";
            const eg::concrete::Action* pParent = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
            VERIFY_RTE( pParent );
            const eg::concrete::Allocator* pAllocator = pParent->getAllocator( pAction );
            VERIFY_RTE( pAllocator );
            const eg::interface::Context* pStaticType = pAction->getContext();
            VERIFY_RTE( pStaticType );
            
            const bool bContextIsWithinComponent = 
                isContextWithinComponent( projectTree, translationUnitAnalysis, pStaticType );
                
            if( const eg::interface::Abstract* pContext = dynamic_cast< const eg::interface::Abstract* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to pause abstract: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Event* pContext = dynamic_cast< const eg::interface::Event* >( pStaticType ) )
            {
                os << strIndent << "ERR( \"Attempted to pause event: " << pAction->getName() << "\" );\n";
            }
            else if( const eg::interface::Function* pContext = dynamic_cast< const eg::interface::Function* >( pStaticType ) )
            {
                VERIFY_RTE( dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) );
                os << strIndent << "ERR( \"Attempted to pause function: " << pAction->getName() << "\" );\n";
            }
            else
            {
                const eg::interface::Action*    pActionContext  = dynamic_cast< const eg::interface::Action* >( pStaticType );
                const eg::interface::Object*    pObjectContext  = dynamic_cast< const eg::interface::Object* >( pStaticType );
                const eg::interface::Link*      pLinkContext    = dynamic_cast< const eg::interface::Link* >( pStaticType );
                
                if( pActionContext || pObjectContext || pLinkContext )
                {
                    const eg::concrete::Dimension_Generated* pStateDim = pAction->getState();
                    VERIFY_RTE( pStateDim );
                    
                    const eg::DataMember* pStateDataMember = layout.getDataMember( pStateDim );
                    VERIFY_RTE( pStateDataMember );
                    
                    os << strIndent << "if( " << *pPrinterFactory->read( pStateDataMember, "reference.instance" ) << 
                        " == " << eg::getActionState( eg::action_paused ) << " )\n";
                    os << strIndent << "{\n";
                    
                    if( bContextIsWithinComponent )
                    {
                        os << strIndent << "    ::eg::Scheduler::unpause_ref( reference );\n";
                    }
                    os << strIndent << "    " << *pPrinterFactory->write( pStateDataMember, "reference.instance" ) << 
                        " = " << eg::getActionState( eg::action_running ) << ";\n";
                    os << strIndent << "}\n";
                }
                else
                {
                    THROW_RTE( "Unknown abstract type" );
                }
            }
    os << "                }\n";
    os << "                break;\n";
        }
    }
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doDone( const eg::reference& reference );
    os << "void PythonInterop::doDone( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    ERR( \"Done used\" );\n";
    os << "}\n";
    
    //virtual void doWaitAction( const eg::reference& reference );
    os << "void PythonInterop::doWaitAction( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    ERR( \"Wait used\" );\n";
    os << "}\n";
    
    //virtual void doWaitDimension( const eg::reference& reference, eg::TypeID dimensionType );
    os << "void PythonInterop::doWaitDimension( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " dimensionType )\n";
    os << "{\n";
    os << "    ERR( \"Wait used\" );\n";
    os << "}\n";
    
    //virtual void doGetAction( const eg::reference& reference );
    os << "void PythonInterop::doGetAction( const " << eg::EG_REFERENCE_TYPE << "& reference )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        pEvaluation->m_result = pybind11::reinterpret_borrow< pybind11::object >( g_pEGRefType->create( reference ) );\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doGetDimension( const eg::reference& reference, eg::TypeID dimensionType );
    os << "void PythonInterop::doGetDimension( const " << eg::EG_REFERENCE_TYPE << "& reference, " << eg::EG_TYPE_ID << " dimensionType )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        switch( dimensionType )\n";
    os << "        {\n";
    
    for( const eg::DataMember* pDataMember : dataMembers )
    {
        if( const eg::concrete::Dimension_User* pDimension = 
            dynamic_cast< const eg::concrete::Dimension_User* >( pDataMember->getInstanceDimension() ) )
        {
    os << "            case " << pDimension->getIndex() << ":\n";
    os << "                pEvaluation->m_result = pybind11::cast( " << *pPrinterFactory->read( pDataMember, "reference.instance" ) << " );\n";
    os << "                break;\n";
        }
    }
    
    os << "            default:\n";
    os << "                break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doRange( eg::EGRangeDescriptionPtr pRange );
    os << "void PythonInterop::doRange( eg::EGRangeDescriptionPtr pRange )\n";
    os << "{\n";
    os << "    if( Evaluation::SharedPtr pEvaluation = m_pEvaluation.lock() )\n";
    os << "    {\n";
    os << "        pEvaluation->m_result = pybind11::make_iterator( megastructure::PythonIterator( *g_pEGRefType, pRange, false ), megastructure::PythonIterator( *g_pEGRefType, pRange, true ) );\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual void doLink( const reference& linkeeRef, TypeID linkeeDimension, const reference& linkValue );
    os << "void PythonInterop::doLink( const eg::reference& linkeeRef, eg::TypeID linkeeDimension, const eg::reference& linkValue )\n";
    os << "{\n";
    os << "    ERR( \"Link used\" );\n";
    os << "}\n";
    
}

void generateRuntimeTypeInterop( std::ostream& os, const eg::ReadSession& session, const megastructure::NetworkAnalysis& networkAnalysis, 
        const Environment& environment, const ProjectTree& projectTree, eg::PrinterFactory::Ptr pPrinterFactory )
{
    const eg::interface::Root* pRoot = session.getTreeRoot();
    
    const eg::DerivationAnalysis& derivationAnalysis = session.getDerivationAnalysis();
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
    std::vector< const eg::concrete::Action* > actions = 
        eg::many_cst< eg::concrete::Action >( objects );
    
    //virtual eg::TimeStamp   getTimestamp( eg::TypeID type, eg::Instance instance );
    os << "eg::TimeStamp PythonInterop::getTimestamp( " << eg::EG_TYPE_ID << " typeID, " << eg::EG_INSTANCE << " instance )\n";
    os << "{\n";
    os << "    switch( typeID )\n";
    os << "    {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getReference() )
        {
    os << "        case " << pAction->getIndex() << ": return " << 
            *pPrinterFactory->read( layout.getDataMember( pAction->getReference() ), "instance" ) << ".data.timestamp;\n";
        }
    }
    os << "        default: throw std::runtime_error( \"Invalid action instance\" );\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual eg::ActionState getState( eg::TypeID type, eg::Instance instance );
    os << eg::EG_ACTION_STATE << " PythonInterop::getState( " << eg::EG_TYPE_ID << " typeID, " << eg::EG_INSTANCE << " instance )\n";
    os << "{\n";
    os << "    switch( typeID )\n";
    os << "    {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getState() )
        {
    os << "        case " << pAction->getIndex() << ": return " << 
        *pPrinterFactory->read( layout.getDataMember( pAction->getState() ), "instance" ) << ";\n";
        }
        else
        {
    os << "        case " << pAction->getIndex() << ": return eg::action_running;\n";
        }
    }
    os << "        default: throw std::runtime_error( \"Invalid action instance\" );\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual eg::TimeStamp   getStopCycle( eg::TypeID type, eg::Instance instance );
    os << eg::EG_TIME_STAMP << " PythonInterop::getStopCycle( " << eg::EG_TYPE_ID << " typeID, " << eg::EG_INSTANCE << " instance )\n";
    os << "{\n";
    os << "    switch( typeID )\n";
    os << "    {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getParent() && pAction->getStopCycle() )
        {
    os << "        case " << pAction->getIndex() << ": return " << 
        *pPrinterFactory->read( layout.getDataMember( pAction->getStopCycle() ), "instance" ) << ";\n";
        }
    }
    os << "        default: throw std::runtime_error( \"Invalid action instance\" );\n";
    os << "    }\n";
    os << "}\n";
    
    //virtual eg::TimeStamp getClockCycle( eg::TypeID typeID );
    os << eg::EG_TIME_STAMP << " PythonInterop::getClockCycle( eg::TypeID typeID )\n";
    os << "{\n";
    os << "    return clock::cycle( typeID );\n";
    os << "}\n\n";
    
    
    const eg::concrete::Action* pInstanceRoot = nullptr;
    {
        std::vector< const eg::concrete::Action* > roots;
        for( const eg::concrete::Element* pChild : session.getInstanceRoot()->getChildren() )
        {
            if( const eg::concrete::Action* pAction = 
                dynamic_cast< const eg::concrete::Action* >( pChild ) )
            {
                roots.push_back( pAction );
            }
        }
        ASSERT( !roots.empty() );
        ASSERT( roots.size() == 1U );
        pInstanceRoot = roots.front();
    }
    
    os << "void* getPythonRoot()\n";
    os << "{\n";
    os << "    return g_pEGRefType->create( " << eg::EG_REFERENCE_TYPE << "{ 0, " << pInstanceRoot->getIndex() << ", 0 } );\n";
    os << "}\n";
}

void generatePythonBindings( std::ostream& os, const eg::ReadSession& session, const megastructure::NetworkAnalysis& networkAnalysis, 
        const Environment& environment, const ProjectTree& projectTree, eg::PrinterFactory::Ptr pPrinterFactory )
{
    os << "#include \"python_lib/python_reference.hpp\"\n";
    os << "#include \"python_lib/python_reference_factory.hpp\"\n";
    os << "#include \"python_lib/python_iterator.hpp\"\n\n";
    os << "#include <pybind11/pybind11.h>\n\n";
    //os << "#include <pybind11/embed.h>\n\n";
    
    os << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
    os << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
    
    /*os << "\n";
    os << "PYBIND11_MODULE( " << projectTree.getComponentFileName( true ) << ", pythonModule )\n";
    os << "{\n";
    os << "    pythonModule.doc() = \"Python bindings for megastructure project\";\n";
    os << "}\n";
    os << "\n";*/

    megastructure::generateRuntimeExterns( os, session );
    
    generateTypeCasters( os, session, environment, projectTree );
    
    generateRuntimeInterface( os, session, environment, projectTree, pPrinterFactory );
    
    generateRuntimeTypeInterop( os, session, networkAnalysis, environment, projectTree, pPrinterFactory );
}
