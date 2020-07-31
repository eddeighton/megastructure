//  Copyright (c) Deighton Systems Limited. 2019. All Rights Reserved.
//  Author: Edward Deighton
//  License: Please see license.txt in the project root folder.

//  Use and copying of this software and preparation of derivative works
//  based upon this software are permitted. Any copy of this software or
//  of any derivative work must include the above copyright notice, this
//  paragraph and the one after it.  Any distribution of this software or
//  derivative works must comply with all applicable laws.

//  This software is made available AS IS, and COPYRIGHT OWNERS DISCLAIMS
//  ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//  PURPOSE, AND NOTWITHSTANDING ANY OTHER PROVISION CONTAINED HEREIN, ANY
//  LIABILITY FOR DAMAGES RESULTING FROM THE SOFTWARE OR ITS USE IS
//  EXPRESSLY DISCLAIMED, WHETHER ARISING IN CONTRACT, TORT (INCLUDING
//  NEGLIGENCE) OR STRICT LIABILITY, EVEN IF COPYRIGHT OWNERS ARE ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGES.

#include "egcomponent/generator.hpp"

#include <iostream>

#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/allocator.hpp"
#include "eg_compiler/translation_unit.hpp"

#include "schema/projectTree.hpp"

namespace megastructure
{
void generateRuntimeExterns( std::ostream& os, const eg::ReadSession& session )
{
    const eg::IndexedObject::Array& objects = 
        session.getObjects( eg::IndexedObject::MASTER_FILE );
    std::vector< const eg::concrete::Action* > actions = 
        eg::many_cst< eg::concrete::Action >( objects );
    
    for( const eg::concrete::Action* pAction : actions )
    {
        if( pAction->getParent() )
        {
            os << "extern " << getStaticType( pAction->getContext() ) << " " << pAction->getName() << "_starter( " << eg::EG_INSTANCE << " );\n";
            os << "extern void " << pAction->getName() << "_stopper( " << eg::EG_INSTANCE << " );\n";
        }
        if( dynamic_cast< const eg::interface::Link* >( pAction->getContext() ) )
        {
            os << "extern void " << pAction->getName() << "_breaker( " << eg::EG_INSTANCE << " );\n";
        }
    }
}
    
static const char* g_NetworkAnalysisRelationTypes[ NetworkAnalysis::TOTAL_RELATION_TYPES ] = 
{
    "eComponent",
    "eProcess",
    "ePlanet"
};

inline std::ostream& operator<<( std::ostream& os, const NetworkAnalysis::BufferRelation type )
{
    return os << g_NetworkAnalysisRelationTypes[ type ];
}

NetworkAnalysis::NetworkAnalysis( const eg::ReadSession& session, const ProjectTree& project )
    :   m_session( session ),
        m_project( project )
{
    getBufferTypes();
    getDataHashBases();
    getHostStructures();
}

void NetworkAnalysis::getBufferTypes()
{
    const eg::Layout& layout = m_session.getLayout();
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        const eg::concrete::Action* pConcreteAction = pBuffer->getAction();
        const eg::interface::Context* pInterfaceAction = pConcreteAction->getContext();
        
        const eg::interface::Root* pCoordinatorRoot = nullptr;
        const eg::interface::Root* pHostNameRoot = nullptr;
        
        const bool bSuccess = pInterfaceAction->getCoordinatorHostname( pCoordinatorRoot, pHostNameRoot );
        VERIFY_RTE( bSuccess );
            
        BufferRelation relation = ePlanet;
        if( pCoordinatorRoot->getIdentifier() == m_project.getCoordinatorName() )
        {
            if( pHostNameRoot->getIdentifier() == m_project.getHostName() )
            {
                relation = eComponent;
            }
            else
            {
                relation = eProcess;
            }
        }
        
        m_bufferTypes[ pBuffer ] = relation;
        
        //std::cout << "Coord: " << m_project.getCoordinatorName() << " Host: " << 
        //    m_project.getHostName() << " buffer: " << pBuffer->getTypeName() << " relation: " << relation << std::endl;
    }             
}

void NetworkAnalysis::getDataHashBases()
{
    m_hashTotal = 0U;
    
    for( BufferTypes::const_iterator 
        i = m_bufferTypes.begin(),
        iEnd = m_bufferTypes.end(); i!=iEnd; ++i )
    {
        const eg::Buffer* pBuffer = i->first;
        BufferRelation relation = i->second;
        
        for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
        {
            switch( relation )
            {
                case eComponent:
                    break;
                case eProcess:
                    if( !pBuffer->isSimple() )
                    {
                        m_hashBases.insert( std::make_pair( pDataMember, m_hashTotal ) );
                        m_hashTotal += pBuffer->getSize();
                    }
                    break;
                case ePlanet:
                    m_hashBases.insert( std::make_pair( pDataMember, m_hashTotal ) );
                    m_hashTotal += pBuffer->getSize();
                    break;
            }
        }
    }
}

const NetworkAnalysis::HostStructures& NetworkAnalysis::getHostStructures( const eg::Buffer* pBuffer ) const
{
    HostNameBufferMap::const_iterator iFind = m_bufferHostNames.find( pBuffer );
    VERIFY_RTE( iFind != m_bufferHostNames.end() );
    
    HostStructureMap::const_iterator iFind2 = m_hostStructures.find( iFind->second );
    VERIFY_RTE( iFind2 != m_hostStructures.end() );
    return iFind2->second;
}

void NetworkAnalysis::getHostStructures()
{
    const eg::Layout& layout = m_session.getLayout();
    //const eg::TranslationUnitAnalysis& translationUnits =
    //  m_session.getTranslationUnitAnalysis();
        
    const eg::interface::Root* pRootRoot = m_session.getTreeRoot();
    VERIFY_RTE( pRootRoot->getRootType() == eg::eInterfaceRoot );
    
    const eg::interface::Root* pRoot = nullptr;
    {
        std::vector< eg::interface::Context* > roots;
        pRootRoot->getChildContexts( roots );
        VERIFY_RTE( roots.size() == 1U );
        pRoot = dynamic_cast< const eg::interface::Root* >( roots.front() );
        VERIFY_RTE( pRoot );
    }
    VERIFY_RTE( pRoot->getRootType() == eg::eMegaRoot );
    
    std::map< const eg::interface::Root*, HostName::Ptr > hostNameMap;
    
    const Coordinator::PtrVector& coordinators = m_project.getCoordinators();
    for( Coordinator::Ptr pCoordinator : coordinators )
    {
        const eg::interface::Root* pCoordinatorContext = dynamic_cast< const eg::interface::Root* >( 
            pRoot->getChildContext( pCoordinator->name() ) );
        VERIFY_RTE( pCoordinatorContext );
        VERIFY_RTE( pCoordinatorContext->getRootType() == eg::eCoordinator );
        
        const HostName::PtrVector& hostNames = pCoordinator->getHostNames();
        for( HostName::Ptr pHostName : hostNames )
        {
            const eg::interface::Root* pHostNameContext = dynamic_cast< const eg::interface::Root* >( 
                pCoordinatorContext->getChildContext( pHostName->name() ) );
            VERIFY_RTE( pHostNameContext );
            VERIFY_RTE( pHostNameContext->getRootType() == eg::eHostName );
            
            //const eg::TranslationUnit* pUnit = translationUnits.getActionTU( pHostNameContext );
            //VERIFY_RTE( pHostNameContext == pUnit->getCoordinatorHostnameDefinitionFile().pHostName );
            
            hostNameMap.insert( std::make_pair( pHostNameContext, pHostName ) );
            
            bool bFoundHostNameProject = false;
            const ProjectName::PtrVector& projectNames = pHostName->getProjectNames();
            for( ProjectName::Ptr pProjectName : projectNames )
            {
                const eg::interface::Root* pProjectContext = dynamic_cast< const eg::interface::Root* >( 
                    pHostNameContext->getChildContext( pProjectName->name() ) );
                VERIFY_RTE( pProjectContext );
                VERIFY_RTE( pProjectContext->getRootType() == eg::eProjectName );
                
                const eg::concrete::Action* pProjectRoot = nullptr;
                {
                    std::vector< const eg::concrete::Element* > instances;
                    m_session.getDerivationAnalysis().getInstances( pHostNameContext, instances, true );
                    VERIFY_RTE( instances.size() == 1U );
                    pProjectRoot = dynamic_cast< const eg::concrete::Action* >( instances.front() );
                    VERIFY_RTE( pProjectRoot );
                }
                
                if( pProjectName->name() == m_project.getProjectName() )
                {
                    if( ( m_project.getCoordinatorName() == pCoordinator->name() ) && 
                        ( m_project.getHostName() == pHostName->name() ) )
                    {
                        //same process - do nothing
                        bFoundHostNameProject = true;
                    }
                    else
                    {
                        HostStructures structures;
                        {
                            std::ostringstream os;
                            os << "e_" << pCoordinator->name() << '_' << pHostName->name();
                            structures.strIdentityEnumName = os.str();
                        }
                        {
                            std::ostringstream os;
                            os << "g_" << pCoordinator->name() << '_' << pHostName->name() << "_writes";
                            structures.strWriteSetName = os.str();
                        }
                        
                        //determine root Run time type
                        structures.pRoot = pProjectRoot;
                        
                        m_hostStructures.insert( std::make_pair( pHostName, structures ) );
                        bFoundHostNameProject = true;
                    }
                    break;
                }
            }
            VERIFY_RTE( bFoundHostNameProject );
        }
    }
    
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        const eg::interface::Context* pBufferContext = pBuffer->getAction()->getContext();
        VERIFY_RTE( pBufferContext );
        bool bFound = false;
        while( pBufferContext )
        {
            if( const eg::interface::Root* pRoot = dynamic_cast< const eg::interface::Root* >( pBufferContext ) )
            {
                if( pRoot->getRootType() == eg::eHostName )
                {
                    std::map< const eg::interface::Root*, HostName::Ptr >::iterator 
                        iFind = hostNameMap.find( pRoot );
                    VERIFY_RTE( iFind != hostNameMap.end() );
                    m_bufferHostNames.insert( std::make_pair( pBuffer, iFind->second ) );
                    bFound = true;
                    break;
                }
            }
            pBufferContext = dynamic_cast< const eg::interface::Context* >( pBufferContext->getParent() );
        }
        VERIFY_RTE( bFound );
        
        const BufferRelation relation = getBufferRelation( pBuffer );
        switch( relation )
        {
            case NetworkAnalysis::eComponent:
                break;
            case NetworkAnalysis::eProcess:
            case NetworkAnalysis::ePlanet:
                {
                    HostNameBufferMap::const_iterator iFind = m_bufferHostNames.find( pBuffer );
                    VERIFY_RTE( iFind != m_bufferHostNames.end() );
                    
                    HostStructureMap::const_iterator iFind2 = m_hostStructures.find( iFind->second );
                    VERIFY_RTE_MSG( iFind2 != m_hostStructures.end(), 
                        "Could not find host structures " << 
                        " buffer: " << pBuffer->getTypeName() << 
                        " host: " << iFind->second->name() << 
                        " for coordinator: " << m_project.getCoordinatorName() << 
                        " hostname: " << m_project.getHostName()
                        );
                }
                break;
            default:
                THROW_RTE( "Unknown buffer relation" );
        }
            
    }
    
}
    

struct MegastructurePrinter : public ::eg::Printer
{
    MegastructurePrinter( const ::eg::DataMember* pDataMember, const char* pszIndex )
         :  m_pDataMember( pDataMember ),
            m_pszIndex( pszIndex)
    {
    }
private:
    const ::eg::DataMember* m_pDataMember;
    const char* m_pszIndex;
protected:
    virtual void print( std::ostream& os ) const
    {
        os << m_pDataMember->getBuffer()->getVariableName() << 
            "[ " << m_pszIndex << " ]." << m_pDataMember->getName();
    }
};


struct MegastructureReader : public ::eg::Printer
{
    MegastructureReader( NetworkAnalysis& networkAnalysis, const ::eg::DataMember* pDataMember, const char* pszIndex )
        :   m_networkAnalysis( networkAnalysis ),
            m_pDataMember( pDataMember ),
            m_pszIndex( pszIndex)
    {
    }
private:
    NetworkAnalysis& m_networkAnalysis;
    const ::eg::DataMember* m_pDataMember;
    const char* m_pszIndex;
protected:
    virtual void print( std::ostream& os ) const
    {
        const eg::Buffer* pBuffer = m_pDataMember->getBuffer();
        VERIFY_RTE( pBuffer );
        const NetworkAnalysis::BufferRelation relation = 
            m_networkAnalysis.getBufferRelation( pBuffer );
            
        switch( relation )
        {
            case NetworkAnalysis::eComponent:
                {
                    os << m_pDataMember->getBuffer()->getVariableName() << 
                        "[ " << m_pszIndex << " ]." << m_pDataMember->getName();
                }
                break;
            case NetworkAnalysis::eProcess:
                {
                    const NetworkAnalysis::HostStructures& hostStructures =
                        m_networkAnalysis.getHostStructures( pBuffer );
                    if( pBuffer->isSimple() )
                    {
                        os << "eg::readlock< ";
                        eg::generateDataMemberType( os, m_pDataMember );
                        os << ", " << hostStructures.strIdentityEnumName << "_read, " << hostStructures.pRoot->getIndex() << " >( " <<
                            m_pDataMember->getBuffer()->getVariableName() << 
                            "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                    }
                    else
                    {
                        const int iHashBash = m_networkAnalysis.getDataMemberReadHashBase( m_pDataMember );
                        os << "eg::readlock< ";
                        eg::generateDataMemberType( os, m_pDataMember );
                        os << ", " << hostStructures.strIdentityEnumName << "_read, " << hostStructures.pRoot->getIndex() << 
                                ", " << iHashBash << ", " << m_pDataMember->getInstanceDimension()->getIndex() << " >( " << m_pszIndex << ", " <<
                            m_pDataMember->getBuffer()->getVariableName() << 
                            "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                    }
                }
                break;
            case NetworkAnalysis::ePlanet:
                {
                    const NetworkAnalysis::HostStructures& hostStructures =
                        m_networkAnalysis.getHostStructures( pBuffer );
                    const int iHashBash = m_networkAnalysis.getDataMemberReadHashBase( m_pDataMember );
                    os << "eg::readlock< ";
                    eg::generateDataMemberType( os, m_pDataMember );
                    os << ", " << hostStructures.strIdentityEnumName << "_read, " << hostStructures.pRoot->getIndex() << 
                            ", " << iHashBash << ", " << m_pDataMember->getInstanceDimension()->getIndex() << " >( " << m_pszIndex << ", " <<
                        m_pDataMember->getBuffer()->getVariableName() << 
                        "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                }
                break;
            default:
                THROW_RTE( "Unknown buffer relation" );
        }
    }
};

struct MegastructureWriter : public ::eg::Printer
{
    MegastructureWriter( NetworkAnalysis& networkAnalysis, const ::eg::DataMember* pDataMember, const char* pszIndex )
        :   m_networkAnalysis( networkAnalysis ),
            m_pDataMember( pDataMember ),
            m_pszIndex( pszIndex)
    {
    }
private:
    NetworkAnalysis& m_networkAnalysis;
    const ::eg::DataMember* m_pDataMember;
    const char* m_pszIndex;
protected:
    virtual void print( std::ostream& os ) const
    {
        const eg::Buffer* pBuffer = m_pDataMember->getBuffer();
        const NetworkAnalysis::BufferRelation relation = 
            m_networkAnalysis.getBufferRelation( pBuffer );
        
        switch( relation )
        {
            case NetworkAnalysis::eComponent:
                {
                    os << m_pDataMember->getBuffer()->getVariableName() << 
                        "[ " << m_pszIndex << " ]." << m_pDataMember->getName();
                }
                break;
            case NetworkAnalysis::eProcess:
                {
                    const NetworkAnalysis::HostStructures& hostStructures =
                        m_networkAnalysis.getHostStructures( pBuffer );
                    if( pBuffer->isSimple() )
                    {
                        os << "eg::writelock< ";
                        eg::generateDataMemberType( os, m_pDataMember );
                        os << ", " << hostStructures.strIdentityEnumName << "_write, " << hostStructures.pRoot->getIndex() << " >( " <<
                            m_pDataMember->getBuffer()->getVariableName() << 
                            "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                    }
                    else
                    {
                        const int iHashBash = m_networkAnalysis.getDataMemberReadHashBase( m_pDataMember );
                        const eg::TypeID dimensionTypeID = m_pDataMember->getInstanceDimension()->getIndex();
                        os << "eg::writelock< ";
                        eg::generateDataMemberType( os, m_pDataMember ); 
                        os << ", " << hostStructures.strIdentityEnumName << "_write, " << 
                            hostStructures.pRoot->getIndex() << ", " << iHashBash << ", " <<
                            dimensionTypeID << " >( " << 
                                m_pszIndex << ", " <<
                                hostStructures.strWriteSetName << ", " <<
                                m_pDataMember->getBuffer()->getVariableName() << "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                    }
                }
                break;
            case NetworkAnalysis::ePlanet:
                {
                    const NetworkAnalysis::HostStructures& hostStructures =
                        m_networkAnalysis.getHostStructures( pBuffer );
                    const int iHashBash = m_networkAnalysis.getDataMemberReadHashBase( m_pDataMember );
                    const eg::TypeID dimensionTypeID = m_pDataMember->getInstanceDimension()->getIndex();
                    os << "eg::writelock< ";
                    eg::generateDataMemberType( os, m_pDataMember ); 
                    os << ", " << hostStructures.strIdentityEnumName << "_write, " << 
                        hostStructures.pRoot->getIndex() << ", " << iHashBash << ", " <<
                        dimensionTypeID << " >( " << 
                            m_pszIndex << ", " <<
                            hostStructures.strWriteSetName << ", " <<
                            m_pDataMember->getBuffer()->getVariableName() << "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                }
                break;
            default:
                THROW_RTE( "Unknown buffer relation" );
        }
    }
};


struct MegastructurePrinterFactory : public ::eg::PrinterFactory
{
    NetworkAnalysis& m_networkAnalysis;
    
    MegastructurePrinterFactory( NetworkAnalysis& networkAnalysis )
        :   m_networkAnalysis( networkAnalysis )
    {
    }
    
    ::eg::Printer::Ptr getPrinter( const ::eg::DataMember* pDataMember, const char* pszIndex )
    {
        return std::make_shared< MegastructurePrinter >( pDataMember, pszIndex );
    }
    ::eg::Printer::Ptr read( const ::eg::DataMember* pDataMember, const char* pszIndex )
    {
        return std::make_shared< MegastructureReader >( m_networkAnalysis, pDataMember, pszIndex );
    }
    ::eg::Printer::Ptr write( const ::eg::DataMember* pDataMember, const char* pszIndex )
    {
        return std::make_shared< MegastructureWriter >( m_networkAnalysis, pDataMember, pszIndex );
    }
};

::eg::PrinterFactory::Ptr NetworkAnalysis::getMegastructurePrinterFactory()
{
    return std::make_shared< MegastructurePrinterFactory >( *this );
}

void recurseEncodeDecode( const ::eg::concrete::Action* pAction, std::ostream& os )
{
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            const std::string strType = getStaticType( pNestedAction->getContext() );
            os << "    template<> inline void encode( Encoder& encoder, const " << strType << "& value ){ encode( encoder, value.data ); }\n";
            os << "    template<> inline void decode( Decoder& decoder, " << strType << "& value )      { decode( decoder, value.data ); }\n";

            recurseEncodeDecode( pNestedAction, os );
        }
    }
}

void recurseEncode( const eg::Layout& layout, const ::eg::concrete::Action* pAction, std::ostream& os )
{
    os << "        case " << pAction->getIndex() << ": ";
    //generateEncode( os, pAction );
    os << " break; //" << pAction->getName() << "\n";
    
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            recurseEncode( layout, pNestedAction, os );
        }
        else if( const eg::concrete::Dimension* pDimension =
            dynamic_cast< const eg::concrete::Dimension* >( pElement ) )
        {
            const eg::DataMember* pDataMember = layout.getDataMember( pDimension );
            
            os << "        case " << pElement->getIndex() << ": ";
            generateEncode( os, pDataMember, "uiInstance" );
            os << " break; //" << pDataMember->getName() << "\n";
        }
    }
}

void recurseDecode( const eg::Layout& layout, const ::eg::concrete::Action* pAction, std::ostream& os )
{
    os << "        case " << pAction->getIndex() << ": ";
    //generateDecode( os, pAction );
    os << "break; //" << pAction->getName() << "\n";
            
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            recurseDecode( layout, pNestedAction, os );
        }
        else if( const eg::concrete::Dimension* pDimension =
            dynamic_cast< const eg::concrete::Dimension* >( pElement ) )
        {
            const eg::DataMember* pDataMember = layout.getDataMember( pDimension );
            os << "        case " << pElement->getIndex() << ": ";
            generateDecode( os, pDataMember, "uiInstance" );
            os << " break; //" << pDataMember->getName() << "\n";
        }
    }
}

void recurseWriteRequest( const NetworkAnalysis& networkAnalysis, const eg::Layout& layout, const ::eg::concrete::Action* pAction, std::ostream& os )
{            
    static const std::string strIndent = "            ";
            
    const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
    for( const eg::concrete::Element* pElement : children )
    {
        if( const eg::concrete::Action* pNestedAction = 
            dynamic_cast< const eg::concrete::Action* >( pElement ) )
        {
            recurseWriteRequest( networkAnalysis, layout, pNestedAction, os );
        }
        else if( const eg::concrete::Dimension* pDimension =
            dynamic_cast< const eg::concrete::Dimension* >( pElement ) )
        {
            if( const eg::concrete::Dimension_Generated* pStateDimension = 
                dynamic_cast< const eg::concrete::Dimension_Generated* >( pDimension ) )
            {
                if( pStateDimension->getDimensionType() == 
                        eg::concrete::Dimension_Generated::eActionState )
                {
                    VERIFY_RTE( pAction == pStateDimension->getAction() );
                    if( pAction->getContext()->isExecutable() && !pAction->getContext()->isMainExecutable() )
                    {
                        
                        const eg::concrete::Dimension_Generated* pReferenceDimension = pAction->getReference();
                        VERIFY_RTE( pReferenceDimension );
                        const eg::concrete::Action* pParent = dynamic_cast< const eg::concrete::Action* >( pAction->getParent() );
                        VERIFY_RTE( pParent );
                        const eg::concrete::Allocator* pAllocator = pParent->getAllocator( pAction );
                        VERIFY_RTE( pAllocator );
                        const eg::interface::Context* pStaticType = pAction->getContext();
                        VERIFY_RTE( pStaticType );
                        
                        const eg::DataMember* pStateDataMember = layout.getDataMember( pStateDimension );
                        const eg::DataMember* pRefDataMember   = layout.getDataMember( pReferenceDimension );
                        eg::Printer::Ptr pStatePrinter  = eg::getDefaultPrinterFactory()->getPrinter( pStateDataMember, "typeInstance.instance" );
                        eg::Printer::Ptr pRefPrinter    = eg::getDefaultPrinterFactory()->getPrinter( pRefDataMember,   "typeInstance.instance" );
                        
                        if( networkAnalysis.isBufferForThisComponent( pStateDataMember->getBuffer() ) )
                        {
            os << strIndent << "case " << pElement->getIndex() << ":\n";
            os << strIndent << "    {\n";
                        
            os << strIndent << "        const auto previousState = " << *pStatePrinter << ";\n";
            os << strIndent << "        decode( typeInstance.type, typeInstance.instance, decoder );\n";
            
                            if( const eg::interface::Abstract* pContext = dynamic_cast< const eg::interface::Abstract* >( pStaticType ) )
                            {
                                THROW_RTE( "Invalid attempt to invoke abstract" );
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
                                    THROW_RTE( "Unreachable" );
                                }
                                else if( pSingletonAllocator || pRangeAllocator )
                                {
                                    THROW_RTE( "TODO - events not supported yet" );
                                    //os << "::eg::Scheduler::signal_ref( ref );";
                                    //os << strIndent << eg::getStaticType( pStaticType ) << " ref = " << pAction->getName() << "_starter( reference.instance );\n";
                                }
                                else
                                {
                                    THROW_RTE( "Unknown allocator type" );
                                }
                            }
                            else if( const eg::interface::Function* pContext = dynamic_cast< const eg::interface::Function* >( pStaticType ) )
                            {
                                THROW_RTE( "Unreachable" );
                            }
                            else
                            {
                                const eg::concrete::SingletonAllocator* pSingletonAllocator =
                                    dynamic_cast< const eg::concrete::SingletonAllocator* >( pAllocator );
                                const eg::concrete::RangeAllocator* pRangeAllocator =
                                    dynamic_cast< const eg::concrete::RangeAllocator* >( pAllocator );
                                    
                                if( const eg::concrete::NothingAllocator* pNothingAllocator =
                                    dynamic_cast< const eg::concrete::NothingAllocator* >( pAllocator ) )
                                {
                                    THROW_RTE( "Unreachable" );
                                }
                                else if( pSingletonAllocator || pRangeAllocator )
                                {
                                    const eg::interface::Action*    pActionContext  = dynamic_cast< const eg::interface::Action* >( pStaticType );
                                    const eg::interface::Object*    pObjectContext  = dynamic_cast< const eg::interface::Object* >( pStaticType );
                                    const eg::interface::Link*      pLinkContext    = dynamic_cast< const eg::interface::Link* >( pStaticType );
                                    
                                    if( pActionContext || pObjectContext || pLinkContext )
                                    {
                                        os << strIndent << "        switch( previousState )\n";
                                        os << strIndent << "        {\n";
                                        os << strIndent << "            case eg::action_stopped: \n";
                                        os << strIndent << "                switch( " << *pStatePrinter << " )\n";
                                        os << strIndent << "                {\n";
                                        os << strIndent << "                    case eg::action_stopped:      break;\n";
                                        if( pStaticType->hasDefinition() )
                                        {
                                        os << strIndent << "                    case eg::action_running:      ::eg::Scheduler::call( " << *pRefPrinter << ", &" << pAction->getName() << "_stopper ); break;\n";
                                        os << strIndent << "                    case eg::action_paused:       ::eg::Scheduler::call( " << *pRefPrinter << ", &" << pAction->getName() << "_stopper );\n";
                                        os << strIndent << "                                                  ::eg::Scheduler::pause_ref( " << *pRefPrinter << ".data ); break;\n";
                                        }
                                        else
                                        {
                                        os << strIndent << "                    case eg::action_running:      ::eg::Scheduler::allocated_ref( " << *pRefPrinter << ".data, &" << pAction->getName() << "_stopper ); break;\n";
                                        os << strIndent << "                    case eg::action_paused:       ::eg::Scheduler::allocated_ref( " << *pRefPrinter << ".data, &" << pAction->getName() << "_stopper );\n";
                                        os << strIndent << "                                                  ::eg::Scheduler::pause_ref( " << *pRefPrinter << ".data ); break;\n";
                                        }
                                        os << strIndent << "                    case eg::TOTAL_ACTION_STATES:\n"; 
                                        os << strIndent << "                        break;\n";
                                        os << strIndent << "                }\n";
                                        os << strIndent << "                break;\n";
                                        os << strIndent << "            case eg::action_running: \n";
                                        os << strIndent << "                switch( " << *pStatePrinter << " )\n";
                                        os << strIndent << "                {\n";
                                        os << strIndent << "                    case eg::action_stopped:      ::eg::Scheduler::stopperStopped( " << *pRefPrinter << ".data ); break;\n";
                                        os << strIndent << "                    case eg::action_running:      break;\n";
                                        os << strIndent << "                    case eg::action_paused:       ::eg::Scheduler::pause_ref( " << *pRefPrinter << ".data ); break;\n";
                                        os << strIndent << "                    case eg::TOTAL_ACTION_STATES:\n";
                                        os << strIndent << "                        break;\n";
                                        os << strIndent << "                }\n";
                                        os << strIndent << "                break;\n";
                                        os << strIndent << "            case eg::action_paused:  \n";
                                        os << strIndent << "                switch( " << *pStatePrinter << " )\n";
                                        os << strIndent << "                {\n";
                                        os << strIndent << "                    case eg::action_stopped:      ::eg::Scheduler::stopperStopped( " << *pRefPrinter << ".data ); break;\n";
                                        os << strIndent << "                    case eg::action_running:      ::eg::Scheduler::unpause_ref( " << *pRefPrinter << ".data ); break;\n";
                                        os << strIndent << "                    case eg::action_paused:       break;\n";
                                        os << strIndent << "                    case eg::TOTAL_ACTION_STATES:\n"; 
                                        os << strIndent << "                        break;\n";
                                        os << strIndent << "                }\n";
                                        os << strIndent << "                break;\n";
                                        os << strIndent << "            case eg::TOTAL_ACTION_STATES:\n"; 
                                        os << strIndent << "                break;\n";
                                        os << strIndent << "        }\n";
                                    }
                                    else
                                    {
                                        THROW_RTE( "Unknown abstract type" );
                                    }
                                }
                                else
                                {
                                    THROW_RTE( "Unknown allocator type" );
                                }
                            }
                        }
                        else
                        {
            os << strIndent << "case " << pElement->getIndex() << ":\n";
            os << strIndent << "    {\n";
            os << strIndent << "        ERR( \"Write request contained type not owned by this component\" );\n";
                        }
            os << strIndent << "    }\n";
            os << strIndent << "    break;\n";
                    }
                }
            }
        }
    }
}



void generate_eg_component( std::ostream& os, 
        const ProjectTree& project,
        const eg::ReadSession& session,
        const NetworkAnalysis& networkAnalysis,
        ComponentType componentType )
{
    
    os << "//ed was here\n";
    os << "#include <chrono>\n";
    os << "#include <thread>\n";
    os << "#include <vector>\n";
    os << "#include <set>\n";

    os << "\n";
    
    os << "#include \"egcomponent/egcomponent.hpp\"\n";
    os << "#include \"egcomponent/traits.hpp\"\n";
    os << "#include \"" << project.getStructuresInclude() << "\"\n";
    os << "#include \"" << project.getNetStateSourceInclude() << "\"\n\n";
    
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
    const eg::concrete::Action* pRoot = session.getInstanceRoot();
    
    const eg::TranslationUnitAnalysis& translationUnitAnalysis =
        session.getTranslationUnitAnalysis();
        
    generateRuntimeExterns( os, session );
        
    os << "\n//network state\n";
    os << "std::bitset< " << networkAnalysis.getReadBitSetSize() << " > g_reads;\n";
    os << "std::bitset< g_TotalHostLocks > g_hostLocks;\n";
    
    const NetworkAnalysis::HostStructureMap& hostStructures = networkAnalysis.getHostStructures();
    
    //define network data structures
    for( const auto& i : hostStructures )
    {
        os << "std::set< eg::TypeInstance > " << i.second.strWriteSetName << ";\n";
    }
    
    os << "\n//buffers\n";
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        if( networkAnalysis.isBufferForThisComponent( pBuffer ) )
        {
            os << "megastructure::SharedBuffer* " << pBuffer->getVariableName() << "_mega;\n";
        }
        else
        {
            os << "megastructure::LocalBuffer* " << pBuffer->getVariableName() << "_mega;\n";
        }
        os << pBuffer->getTypeName() << "* " << pBuffer->getVariableName() << " = nullptr;\n";
    }
    os << "\n";
    
    os << "void allocate_buffers( megastructure::MemorySystem* pMemorySystem )\n";
    os << "{\n";
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        if( networkAnalysis.isBufferForThisComponent( pBuffer ) )
        {
            os << "    " << pBuffer->getVariableName() << "_mega = pMemorySystem->getSharedBuffer( \"" << 
                pBuffer->getVariableName() << "\" , " <<  pBuffer->getSize() << " * sizeof( " << pBuffer->getTypeName() << " ) );\n";
        }
        else
        {
            os << "    " << pBuffer->getVariableName() << "_mega = pMemorySystem->getLocalBuffer( \"" << 
                pBuffer->getVariableName() << "\" , " <<  pBuffer->getSize() << " * sizeof( " << pBuffer->getTypeName() << " ) );\n";
        }
        os << "    " << pBuffer->getVariableName() << " = reinterpret_cast< " << pBuffer->getTypeName() << "* >( " << pBuffer->getVariableName() << "_mega->getData() );\n";

    os << "    for( " << eg::EG_INSTANCE << " i = 0U; i != " << pBuffer->getSize() << "; ++i )\n";
    os << "    {\n";
        for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
        {
            eg::generateAllocation( os, pDataMember, "i" ); 
        }
    
    os << "    }\n";
    }
    os << "}\n";
    
    os << "\n";
    
    os << "void deallocate_buffers(  megastructure::MemorySystem* pMemorySystem )\n";
    os << "{\n";
    //deallocate in reverse
    for( std::size_t sz = layout.getBuffers().size(); sz > 0U; --sz )
    {
        const eg::Buffer* pBuffer = layout.getBuffers()[ sz - 1U ];
        os << "    " << pBuffer->getVariableName() << "_mega->Release();\n";

    os << "    for( " << eg::EG_INSTANCE << " i = 0U; i != " << pBuffer->getSize() << "; ++i )\n";
    os << "    {\n";
        for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
        {
            eg::generateDeallocation( os, pDataMember, "i" ); 
        }
    os << "    }\n";
    }
    os << "}\n";
    os << "\n";
    os << "namespace eg\n";
    os << "{\n";
    recurseEncodeDecode( pRoot, os );
    os << "}\n";
    os << "\n";
    
    os << "\n//encode decode\n";
    os << "void encode( std::int32_t iType, std::uint32_t uiInstance, eg::Encoder& buffer )\n";
    os << "{\n";
    os << "    switch( iType )\n";
    os << "    {\n";
    recurseEncode( layout, pRoot, os );
    os << "        default: \n";
    os << "        {\n";
    os << "            std::ostringstream _os;\n";
    os << "            _os << \"encode error: unknown type: \" << iType << \" instance: \" << uiInstance;\n";
    os << "            throw std::runtime_error( _os.str() );\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    os << "void decode( std::int32_t iType, std::uint32_t uiInstance, eg::Decoder& buffer )\n";
    os << "{\n";
    os << "    switch( iType )\n";
    os << "    {\n";
    recurseDecode( layout, pRoot, os );
    os << "        default: \n";
    os << "        {\n";
    os << "            std::ostringstream _os;\n";
    os << "            _os << \"decode error: unknown type: \" << iType << \" instance: \" << uiInstance;\n";
    os << "            throw std::runtime_error( _os.str() );\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    os << "\n";
    
    
    os << "void completeSimulationLocks( megastructure::MegaProtocol* pMegaProtocol )\n";
    os << "{\n";
    
    for( const auto& i : hostStructures )
    {
    os << "    if( g_hostLocks.test( " << i.second.strIdentityEnumName << "_write ) )\n";
    os << "    {\n";
    
    os << "        msgpack::sbuffer buffer;\n";
    os << "        msgpack::packer< msgpack::sbuffer > encoder( &buffer );\n";
    os << "        for( const eg::TypeInstance& typeInstance : " << i.second.strWriteSetName << " )\n";
    os << "        {\n";
    os << "            eg::encode( encoder, typeInstance.type );\n";
    os << "            eg::encode( encoder, typeInstance.instance );\n";
    os << "            encode( typeInstance.type, typeInstance.instance, encoder );\n";
    os << "        }\n";
    os << "        pMegaProtocol->write( " << i.second.pRoot->getIndex() << ", buffer.data(), buffer.size(), clock::cycle() );\n";
    os << "    }\n";
    }
    
    os << "\n";
    os << "    g_reads.reset();\n";
    os << "    g_hostLocks.reset();\n";
    for( const auto& i : hostStructures )
    {
        os << "    " << i.second.strWriteSetName << ".clear();\n";
    }
    os << "}\n";
    
    os << "void decodeWriteRequest( eg::Decoder& decoder )\n";
    os << "{\n";
    os << "    eg::TypeInstance typeInstance;\n";
    os << "    msgpack::object_handle objectHandle;\n";
    os << "    while( decoder.next( objectHandle ) )\n";
    os << "    {\n";
    os << "        objectHandle.get().convert( typeInstance.type );\n";
    os << "        eg::decode( decoder, typeInstance.instance );\n";
    os << "        switch( typeInstance.type )\n";
    os << "        {\n";
    recurseWriteRequest( networkAnalysis, layout, pRoot, os );
    os << "            default:\n";
    os << "               decode( typeInstance.type, typeInstance.instance, decoder );\n";
    os << "               break;\n";
    os << "        }\n";
    os << "    }\n";
    os << "}\n";
    
    if( eComponent_Python == componentType )
    {
    os << "extern eg::ComponentInterop& getPythonInterop();\n";
    os << "extern void setEGRuntime( eg::EGRuntime& egRuntime );\n";
    os << "extern void* getPythonRoot();\n";
    }
    else if( eComponent_Unreal == componentType )
    {
    os << "extern void* getUnrealRoot();\n";
    }

    const char szComponentPart1[] = R"(
    
namespace megastructure
{
    
class EGComponentImpl : public EGComponent, public EncodeDecode
{
    MemorySystem* m_pMemorySystem = nullptr;
    MegaProtocol* m_pMegaProtocol = nullptr;
    eg::EGRuntimePtr m_pEGRuntime;
    void* m_pHostInterface = nullptr;
public:
    virtual ~EGComponentImpl()
    {
    }
    
)";
    os << szComponentPart1 << "\n";
    
    os << "    virtual void Initialise( void* pHostInterface, EncodeDecode*& pEncodeDecode, MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol, const char* pszDataBasePath )\n";
    os << "    {\n";
    os << "        pEncodeDecode = this;\n";
    os << "        \n";
    os << "        m_pMemorySystem = pMemorySystem;\n";
    os << "        m_pMegaProtocol = pMegaProtocol;\n";
    
    if( eComponent_Python == componentType )
    {
    os << "        eg::ComponentInterop& interop = getPythonInterop();\n";
    os << "        m_pEGRuntime = eg::constructRuntime( interop, pszDataBasePath );\n";
    os << "        setEGRuntime( *m_pEGRuntime );\n";
    }
    
    if( eComponent_Unreal == componentType )
    {
    os << "        m_pHostInterface = pHostInterface;\n";
    }
    
    os << "\n";
    os << "        allocate_buffers( m_pMemorySystem );\n";
    os << "    }\n";
    
    
    if( eComponent_Python == componentType )
    {
    os << "    virtual void* GetRoot()\n";
    os << "    {\n";
    os << "        return getPythonRoot();\n";
    os << "    }\n";
    }
    else if( eComponent_Unreal == componentType )
    {
    os << "    virtual void* GetRoot()\n";
    os << "    {\n";
    os << "        return getUnrealRoot();\n";
    os << "    }\n";
    }
    else
    {
    os << "    virtual void* GetRoot()\n";
    os << "    {\n";
    os << "        return nullptr;\n";
    os << "    }\n";
    }
    
    
    
const char szComponentPart2[] = R"(
    
    virtual void Uninitialise()
    {
        deallocate_buffers( m_pMemorySystem );
    }
    
    virtual void Cycle()
    {
        eg::Scheduler::cycle();
        completeSimulationLocks( m_pMegaProtocol );
        clock::next();
    }
    
    virtual void encode( std::int32_t iType, std::uint32_t uiInstance, eg::Encoder& buffer )
    {
        ::encode( iType, uiInstance, buffer );
    }
    
    virtual void decode( std::int32_t iType, std::uint32_t uiInstance, eg::Decoder& decoder )
    {
        ::decode( iType, uiInstance, decoder );
    }
    
	virtual void decode( eg::Decoder& decoder )
    {
        ::decodeWriteRequest( decoder );
    }
    
    
    //networking
    void readlock( eg::TypeID component )
    {
        m_pMegaProtocol->readlock( component, clock::cycle() );
    }
    
    void read( eg::TypeID type, eg::Instance instance )
    {
        m_pMegaProtocol->read( type, instance, clock::cycle() );
    }

    void writelock( eg::TypeID component )
    {
        m_pMegaProtocol->writelock( component, clock::cycle() );
    }

    void* getHostInterface() const
    {
        return m_pHostInterface;
    }

};

extern "C" BOOST_SYMBOL_EXPORT EGComponentImpl g_pluginSymbol;
EGComponentImpl g_pluginSymbol;

}
    
    )";
    
    os << szComponentPart2 << "\n";
    
    const char szEventRoutines[] = R"(
    
    
eg::event_iterator events::getIterator()
{
    return eg::event_iterator{};
}

bool events::get( eg::event_iterator& iterator, Event& event )
{
    return false;
}

void events::put( const char* type, eg::TimeStamp timestamp, const void* value, std::size_t size )
{
}
    
bool events::update()
{
    return true;
}

namespace eg
{
    void Component::readlock( eg::TypeID component )
    {
        megastructure::g_pluginSymbol.readlock( component );
    }

    void Component::read( eg::TypeID type, eg::Instance instance )
    {
        megastructure::g_pluginSymbol.read( type, instance );
    }

    void Component::writelock( eg::TypeID component )
    {
        megastructure::g_pluginSymbol.writelock( component );
    }
}

    )";
    os << szEventRoutines << "\n";
    
    if( eComponent_Unreal == componentType )
    {
        
    const char szUnrealStuff[] = R"(
    
namespace Unreal
{
    struct IEngineInterface
    {
        virtual ~IEngineInterface(){}
        virtual void log( const wchar_t* msg ) = 0;
    };
    
    void log( const wchar_t* msg )
    {
        IEngineInterface* pUnreal = reinterpret_cast< IEngineInterface* >( 
            megastructure::g_pluginSymbol.getHostInterface() );
        pUnreal->log( msg );
    }
}
    
    )";
    os << szUnrealStuff << "\n";
    }
    else
    {
        //fake it
    const char szUnrealStuff[] = R"(
    
namespace Unreal
{
    void log( const wchar_t* msg )
    {
        ERR( "Unreal interface used outside unreal component" );
    }
}
    
    )";
    os << szUnrealStuff << "\n";
    }
        
    
}

} //namespace megastructure