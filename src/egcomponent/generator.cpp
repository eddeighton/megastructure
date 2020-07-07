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
#include "eg_compiler/translation_unit.hpp"

#include "schema/projectTree.hpp"

namespace megastructure
{
    
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
	//	m_session.getTranslationUnitAnalysis();
        
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
                        {
                            std::ostringstream os;
                            os << "g_" << pCoordinator->name() << '_' << pHostName->name() << "_activations";
                            structures.strActivationSetName = os.str();
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
                        const std::string& strSet = m_pDataMember->isActivationState() ? 
                            hostStructures.strActivationSetName : hostStructures.strWriteSetName;
                        
                        const eg::TypeID dimensionTypeID = m_pDataMember->getInstanceDimension()->getIndex();
                        os << "eg::writelock< ";
                        eg::generateDataMemberType( os, m_pDataMember ); 
                        os << ", " << hostStructures.strIdentityEnumName << "_write, " << 
                            hostStructures.pRoot->getIndex() << ", " << 
                            dimensionTypeID << " >( " << 
                                m_pszIndex << ", " <<
                                strSet << ", " <<
                                m_pDataMember->getBuffer()->getVariableName() << "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
                    }
                }
                break;
            case NetworkAnalysis::ePlanet:
                {
                    const NetworkAnalysis::HostStructures& hostStructures =
                        m_networkAnalysis.getHostStructures( pBuffer );
                        
                    const std::string& strSet = m_pDataMember->isActivationState() ? 
                        hostStructures.strActivationSetName : hostStructures.strWriteSetName;
                        
                    const eg::TypeID dimensionTypeID = m_pDataMember->getInstanceDimension()->getIndex();
                    os << "eg::writelock< ";
                    eg::generateDataMemberType( os, m_pDataMember ); 
                    os << ", " << hostStructures.strIdentityEnumName << "_write, " << 
                        hostStructures.pRoot->getIndex() << ", " << 
                        dimensionTypeID << " >( " << 
                            m_pszIndex << ", " <<
                            strSet << ", " <<
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
			os << "    template<> inline void decode( Decoder& decoder, " << strType << "& value ) 		{ decode( decoder, value.data ); }\n";

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

void generate_eg_component( std::ostream& os, 
		const ProjectTree& project,
		const eg::ReadSession& session,
        const NetworkAnalysis& networkAnalysis )
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
    os << "#include \"" << project.getNetStateSourceInclude() << "\"\n";
	
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
	const eg::concrete::Action* pRoot = session.getInstanceRoot();
	
    const eg::TranslationUnitAnalysis& translationUnitAnalysis =
		session.getTranslationUnitAnalysis();
        
        
    os << "\n//network state\n";
    os << "std::bitset< " << networkAnalysis.getReadBitSetSize() << " > g_reads;\n";
    os << "std::bitset< g_TotalHostLocks > g_hostLocks;\n";
    
    const NetworkAnalysis::HostStructureMap& hostStructures = networkAnalysis.getHostStructures();
    
    //define network data structures
    for( const auto& i : hostStructures )
    {
        os << "std::set< eg::TypeInstance > " << i.second.strWriteSetName << ";\n";
        os << "std::set< eg::TypeInstance > " << i.second.strActivationSetName << ";\n";
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
	os << "            _os << \"Unknown type: \" << iType << \" instance: \" << uiInstance;\n";
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
	os << "            _os << \"Unknown type: \" << iType << \" instance: \" << uiInstance;\n";
	os << "            throw std::runtime_error( _os.str() );\n";
	os << "        }\n";
	os << "    }\n";
	os << "}\n";
    os << "\n";
	

	const char szStuff[] = R"(
    
    
	
namespace megastructure
{
	
class EGComponentImpl : public EGComponent, public EncodeDecode
{
	MemorySystem* m_pMemorySystem = nullptr;
	MegaProtocol* m_pMegaProtocol = nullptr;
public:
	virtual ~EGComponentImpl()
	{
	}
	
	virtual void Initialise( EncodeDecode*& pEncodeDecode, MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol )
	{
		pEncodeDecode = this;
		
		m_pMemorySystem = pMemorySystem;
		m_pMegaProtocol = pMegaProtocol;
		
		allocate_buffers( m_pMemorySystem );
	}
	virtual void Uninitialise()
	{
		deallocate_buffers( m_pMemorySystem );
	}
	
	virtual void Cycle()
	{
        eg::Scheduler::cycle();
		clock::next();
	}
	
	virtual void encode( std::int32_t iType, std::uint32_t uiInstance, eg::Encoder& buffer )
	{
		::encode( iType, uiInstance, buffer );
	}
	
	virtual void decode( std::int32_t iType, std::uint32_t uiInstance, eg::Decoder& buffer )
	{
		::decode( iType, uiInstance, buffer );
	}
    
    
    //networking
    void readlock( eg::TypeID component )
    {
        
    }

    void writelock( eg::TypeID component )
    {
        
    }

    void read( eg::TypeID component, eg::TypeID type, eg::Instance instance )
    {
        
    }

};

extern "C" BOOST_SYMBOL_EXPORT EGComponentImpl g_pluginSymbol;
EGComponentImpl g_pluginSymbol;

}
	
	)";
	
	os << szStuff << "\n";
	
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

    void Component::writelock( eg::TypeID component )
    {
        megastructure::g_pluginSymbol.writelock( component );
    }

    void Component::read( eg::TypeID component, eg::TypeID type, eg::Instance instance )
    {
        megastructure::g_pluginSymbol.read( component, type, instance );
    }
}

	)";
	os << szEventRoutines << "\n";
}

} //namespace megastructure