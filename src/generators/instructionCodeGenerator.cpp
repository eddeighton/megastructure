
#include "egcomponent/generator.hpp"

#include "schema/projectTree.hpp"

#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

#include "eg_compiler/allocator.hpp"
#include "eg_compiler/translation_unit.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

#include "common/assert_verify.hpp"

namespace eg
{
    class MegaInstructionCodeGenerator : public InstructionCodeGenerator
    {
        const megastructure::NetworkAnalysis& m_networkAnalysis;
        const eg::TranslationUnitAnalysis& m_translationUnitAnalysis;
        const ProjectTree& m_projectTree;
    public:
        MegaInstructionCodeGenerator( CodeGenerator& _generator, std::ostream& _os, 
                    const megastructure::NetworkAnalysis& networkAnalysis,
                    const eg::TranslationUnitAnalysis& translationUnitAnalysis,
                    const ProjectTree& projectTree )
            :   InstructionCodeGenerator( _generator, _os ),
                m_networkAnalysis( networkAnalysis ),
                m_translationUnitAnalysis( translationUnitAnalysis ),
                m_projectTree( projectTree )
        {}
        
        bool isContextWithinComponent( const interface::Context* pContext ) const
        {
            const eg::TranslationUnit* pTU = m_translationUnitAnalysis.getActionTU( pContext );
            return pTU->getCoordinatorHostnameDefinitionFile().isCoordinator( m_projectTree.getCoordinatorName() ) &&
                pTU->getCoordinatorHostnameDefinitionFile().isHost( m_projectTree.getHostName() );
        }
        
        void generate( const CallOperation& ins )
        {
            const concrete::Action* pParent = dynamic_cast< const concrete::Action* >( ins.getConcreteType()->getParent() );
            VERIFY_RTE( pParent );
            const concrete::Allocator* pAllocator = pParent->getAllocator( ins.getConcreteType() );
            VERIFY_RTE( pAllocator );
            const interface::Context* pStaticType = ins.getConcreteType()->getContext();
            VERIFY_RTE( pStaticType );
            
            //determine whether event is local to component
            const bool bContextIsWithinComponent  = isContextWithinComponent( pStaticType );
            
            //acquire the object via the starter / allocator
            if( const interface::Abstract* pContext = dynamic_cast< const interface::Abstract* >( pStaticType ) )
            {
                THROW_RTE( "Invalid attempt to invoke abstract" );
            }
            else if( const interface::Event* pContext = dynamic_cast< const interface::Event* >( pStaticType ) )
            {
                const concrete::SingletonAllocator* pSingletonAllocator =
                    dynamic_cast< const concrete::SingletonAllocator* >( pAllocator );
                const concrete::RangeAllocator* pRangeAllocator =
                    dynamic_cast< const concrete::RangeAllocator* >( pAllocator );
                    
                if( const concrete::NothingAllocator* pNothingAllocator =
                    dynamic_cast< const concrete::NothingAllocator* >( pAllocator ) )
                {
                    os << generator.getIndent() << getStaticType( pStaticType ) << " ref = eg::reference{ " << generator.getVarExpr( ins.getInstance() ) << 
                        ", " << ins.getConcreteType()->getIndex() << ", clock::cycle() };\n";
                    
                    if( bContextIsWithinComponent )
                    {
                    os << generator.getIndent() << "::eg::Scheduler::signal_ref( ref );\n";
                    }  
                    
                    os << generator.getIndent() << "return ref;\n";
                }
                else if( pSingletonAllocator || pRangeAllocator )
                {
                    os << generator.getIndent() << getStaticType( pStaticType ) << " ref = " << ins.getConcreteType()->getName() << 
                        "_starter( " << generator.getVarExpr( ins.getInstance() ) << " );\n";
                    os << generator.getIndent() << "if( ref )\n";
                    os << generator.getIndent() << "{\n";
                    
                    if( bContextIsWithinComponent )
                    {
                    os << generator.getIndent() << "    ::eg::Scheduler::signal_ref( ref );\n";
                    os << generator.getIndent() << "    " << ins.getConcreteType()->getName() << "_stopper( ref.data.instance );\n";
                    }
                    
                    os << generator.getIndent() << "}\n";
                    os << generator.getIndent() << "else\n";
                    os << generator.getIndent() << "{\n";
                    os << generator.getIndent() << "    ERR( \"Failed to allocate action: " << ins.getConcreteType()->getName() << "\" );\n";
                    os << generator.getIndent() << "}\n";
                    os << generator.getIndent() << "return ref;\n";
                }
                else
                {
                    THROW_RTE( "Unknown allocator type" );
                }
            }
            else if( const interface::Function* pContext = dynamic_cast< const interface::Function* >( pStaticType ) )
            {
                VERIFY_RTE( dynamic_cast< const concrete::NothingAllocator* >( pAllocator ) );
                //directly invoke the function
                os << generator.getIndent() << getStaticType( pStaticType ) << " ref = eg::reference{ " << generator.getVarExpr( ins.getInstance() ) << 
                    ", " << ins.getConcreteType()->getIndex() << ", clock::cycle() };\n";
                os << generator.getIndent() << "return ref( args... );\n";
            }
            else
            {
                const interface::Action*    pActionContext  = dynamic_cast< const interface::Action* >( pStaticType );
                const interface::Object*    pObjectContext  = dynamic_cast< const interface::Object* >( pStaticType );
                const interface::Link*      pLinkContext    = dynamic_cast< const interface::Link* >( pStaticType );
                
                if( pActionContext || pObjectContext || pLinkContext )
                {
                    os << generator.getIndent() << getStaticType( pStaticType ) << " ref = " << ins.getConcreteType()->getName() << 
                        "_starter( " << generator.getVarExpr( ins.getInstance() ) << " );\n";
                        
                    if( pStaticType->hasDefinition() )
                    {
                        os << generator.getIndent() << "if( ref )\n";
                        os << generator.getIndent() << "{\n";
                        if( bContextIsWithinComponent )
                        {
                        os << generator.getIndent() << "    ::eg::Scheduler::call( ref, &" << ins.getConcreteType()->getName() << "_stopper, args... );\n";
                        }
                        os << generator.getIndent() << "}\n";
                        os << generator.getIndent() << "else\n";
                        os << generator.getIndent() << "{\n";
                        os << generator.getIndent() << "    ERR( \"Failed to allocate action: " << ins.getConcreteType()->getName() << "\" );\n";
                        os << generator.getIndent() << "}\n";
                        os << generator.getIndent() << "return ref;\n";
                    }
                    else
                    {
                        os << generator.getIndent() << "if( ref )\n";
                        os << generator.getIndent() << "{\n";
                        if( bContextIsWithinComponent )
                        {
                        os << generator.getIndent() << "    ::eg::Scheduler::allocated_ref( ref.data, &" << ins.getConcreteType()->getName() << "_stopper );\n";
                        }
                        os << generator.getIndent() << "}\n";
                        os << generator.getIndent() << "else\n";
                        os << generator.getIndent() << "{\n";
                        os << generator.getIndent() << "    ERR( \"Failed to allocate action: " << ins.getConcreteType()->getName() << "\" );\n";
                        os << generator.getIndent() << "}\n";
                        os << generator.getIndent() << "return ref;\n";
                    }
                }
                else
                {
                    THROW_RTE( "Unknown abstract type" );
                }
            }
        }
        void generate( const StartOperation& ins )
        {
            THROW_RTE( "TODO StartOperation" );
        }
        void generate( const StopOperation& ins )
        {
            //determine whether event is local to component
            const interface::Context* pStaticType = ins.getConcreteType()->getContext();
            VERIFY_RTE( pStaticType );
            const bool bContextIsWithinComponent  = isContextWithinComponent( pStaticType );
            
            const concrete::Dimension_Generated* pReferenceDimension = ins.getConcreteType()->getReference();
            VERIFY_RTE( pReferenceDimension );
            
            if( bContextIsWithinComponent )
            {
                os << generator.getIndent() << "::eg::Scheduler::stop_ref( " << 
                    *generator.read( pReferenceDimension, generator.getVarExpr( ins.getInstance() ) ) << ".data );\n";
            }
            else
            {
                //directly invoke the stopper
                os << ins.getConcreteType()->getName() << "_stopper( " << ins.getInstance() << ".data.instance );\n";
            }
        }
        void generate( const PauseOperation& ins )
        {
            //determine whether event is local to component
            const interface::Context* pStaticType = ins.getConcreteType()->getContext();
            VERIFY_RTE( pStaticType );
            const bool bContextIsWithinComponent  = isContextWithinComponent( pStaticType );
            
            const concrete::Dimension_Generated* pReferenceDimension = ins.getConcreteType()->getReference();
            VERIFY_RTE( pReferenceDimension );
            os << generator.getIndent() << "if( " << 
                *generator.read( ins.getConcreteType()->getState(), generator.getVarExpr( ins.getInstance() ) ) <<
                    " == " << getActionState( action_running ) << " )\n";
            os << generator.getIndent() << "{\n";
            if( bContextIsWithinComponent )
            {
                os << generator.getIndent() << "    ::eg::Scheduler::pause_ref( " << 
                    *generator.read( pReferenceDimension, generator.getVarExpr( ins.getInstance() ) ) << ".data );\n";
            }
            os << generator.getIndent() << "    " <<
                *generator.write( ins.getConcreteType()->getState(), generator.getVarExpr( ins.getInstance() ) ) << 
                    " = " << getActionState( action_paused ) << ";\n";
            os << generator.getIndent() << "}\n";
        }
        void generate( const ResumeOperation& ins )
        {
            //determine whether event is local to component
            const interface::Context* pStaticType = ins.getConcreteType()->getContext();
            VERIFY_RTE( pStaticType );
            const bool bContextIsWithinComponent  = isContextWithinComponent( pStaticType );
            
            const concrete::Dimension_Generated* pReferenceDimension = ins.getConcreteType()->getReference();
            VERIFY_RTE( pReferenceDimension );
            os << generator.getIndent() << "if( " << 
                *generator.read( ins.getConcreteType()->getState(), generator.getVarExpr( ins.getInstance() ) ) <<
                    " == " << getActionState( action_paused ) << " )\n";
            os << generator.getIndent() << "{\n";
            if( bContextIsWithinComponent )
            {
                os << generator.getIndent() << "    ::eg::Scheduler::unpause_ref( " << 
                    *generator.read( pReferenceDimension, generator.getVarExpr( ins.getInstance() ) ) << ".data );\n";
            }
            os << generator.getIndent() << "    " <<
                *generator.write( ins.getConcreteType()->getState(), generator.getVarExpr( ins.getInstance() ) ) << 
                    " = " << getActionState( action_running ) << ";\n";
            os << generator.getIndent() << "}\n";
        }
    };
}

namespace megastructure
{   
    InstructionCodeGeneratorFactoryImpl::InstructionCodeGeneratorFactoryImpl( 
                const megastructure::NetworkAnalysis& networkAnalysis,
                const eg::TranslationUnitAnalysis& translationUnitAnalysis,
                const ProjectTree& projectTree )
        :   m_networkAnalysis( networkAnalysis ),
            m_translationUnitAnalysis( translationUnitAnalysis ),
            m_projectTree( projectTree )
    {
    }
        
    std::shared_ptr< eg::InstructionCodeGenerator > InstructionCodeGeneratorFactoryImpl::create( eg::CodeGenerator& generator, std::ostream& os )
    {
        return std::make_shared< eg::MegaInstructionCodeGenerator >( generator, os, 
            m_networkAnalysis, m_translationUnitAnalysis, m_projectTree );
    }
}
