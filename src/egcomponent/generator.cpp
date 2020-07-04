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

NetworkAnalysis::NetworkAnalysis( const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, const ProjectTree& project )
    :   m_project( project )
{
    getBufferTypes( layout, translationUnitAnalysis, m_project.getCoordinatorName(), m_project.getHostName() );
    getDataHashBases( layout, translationUnitAnalysis, m_project.getCoordinatorName(), m_project.getHostName() );
    getHostStructures( m_project );
}


void NetworkAnalysis::getBufferTypes( const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
	const std::string& strCoordinator, const std::string& strHost )
{
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        const eg::concrete::Action* pConcreteAction = pBuffer->getAction();
        const eg::interface::Context* pInterfaceAction = pConcreteAction->getContext();
        const eg::TranslationUnit* pUnit = translationUnitAnalysis.getActionTU( pInterfaceAction );
            
        if( pUnit->getCoordinatorHostnameDefinitionFile().isCoordinator( strCoordinator ) )
        {
            if( pUnit->getCoordinatorHostnameDefinitionFile().isHost( strHost ) )
            {
                m_bufferTypes[ pBuffer ] = eComponent;
            }
            else
            {
                m_bufferTypes[ pBuffer ] = eProcess;
            }
        }
        else
        {
            m_bufferTypes[ pBuffer ] = ePlanet;
        }
    }
}


void NetworkAnalysis::getDataHashBases( const eg::Layout& layout, 
    const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
	const std::string& strCoordinator, const std::string& strHost )
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
                    m_hashBases.insert( std::make_pair( pDataMember, eNothing ) );
                    break;
                case eProcess:
                    m_hashBases.insert( std::make_pair( pDataMember, eSimLock ) );
                    break;
                case ePlanet:
                    m_hashBases.insert( std::make_pair( pDataMember, m_hashTotal ) );
                    m_hashTotal += pBuffer->getSize();
                    break;
            }
        }
    }
}

void NetworkAnalysis::getHostStructures( const ProjectTree& project )
{
    const Coordinator::PtrVector& coordinators = project.getCoordinators();
    for( Coordinator::Ptr pCoordinator : coordinators )
    {
        const HostName::PtrVector& hostNames = pCoordinator->getHostNames();
        for( HostName::Ptr pHostName : hostNames )
        {
            const ProjectName::PtrVector& projectNames = pHostName->getProjectNames();
            for( ProjectName::Ptr pProjectName : projectNames )
            {
                if( pProjectName->name() == project.getProjectName() )
                {
                    if( ( project.getCoordinatorName() == pCoordinator->name() ) && 
                        ( project.getHostName() == pHostName->name() ) )
                    {
                        //same process - do nothing
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
                            os << "std::set< eg::TypeInstance > g_" << pCoordinator->name() << '_' << pHostName->name() << "_writes";
                            structures.strWriteSetName = os.str();
                        }
                        {
                            std::ostringstream os;
                            os << "std::set< eg::TypeInstance > g_" << pCoordinator->name() << '_' << pHostName->name() << "_activations";
                            structures.strActivationSetName = os.str();
                        }
                        m_hostStructures.insert( std::make_pair( pHostName, structures ) );
                    }
                    break;
                }
            }
        }
    }
}
    

struct MegastructurePrinter : public ::eg::Printer
{
    MegastructurePrinter( NetworkAnalysis& networkAnalysis, const ::eg::DataMember* pDataMember, const char* pszIndex )
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
        
        const int iHashBash = m_networkAnalysis.getDataMemberReadHashBase( m_pDataMember );
        if( iHashBash == NetworkAnalysis::eNothing )
        {
            os << m_pDataMember->getBuffer()->getVariableName() << 
                "[ " << m_pszIndex << " ]." << m_pDataMember->getName();
        }
        /*else if( iHashBash == NetworkAnalysis::eSimLock )
        {
            
        }*/
        else
        {
            os << "eg::read< "; generateDataMemberType( os, m_pDataMember ); os << ", " << 
                iHashBash << " >( " <<
                m_pDataMember->getInstanceDimension()->getIndex() << ", " << m_pszIndex << ", " <<
                m_pDataMember->getBuffer()->getVariableName() << 
                "[ " << m_pszIndex << " ]." << m_pDataMember->getName() << " )";
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
        return std::make_shared< MegastructurePrinter >( m_networkAnalysis, pDataMember, pszIndex );
    }
    ::eg::Printer::Ptr read( const ::eg::DataMember* pDataMember, const char* pszIndex )
    {
        return std::make_shared< MegastructurePrinter >( m_networkAnalysis, pDataMember, pszIndex );
    }
    ::eg::Printer::Ptr write( const ::eg::DataMember* pDataMember, const char* pszIndex )
    {
        return std::make_shared< MegastructurePrinter >( m_networkAnalysis, pDataMember, pszIndex );
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
    
    const NetworkAnalysis::HostStructureMap& hostStructures = networkAnalysis.getHostStructures();
    
    //define network data structures
    for( const auto& i : hostStructures )
    {
        os << i.second.strWriteSetName << ";\n";
        os << i.second.strActivationSetName << ";\n";
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
	
	virtual void WaitForReadResponse( std::int32_t iType, std::uint32_t uiInstance )
	{
	}
	
	virtual void encode( std::int32_t iType, std::uint32_t uiInstance, eg::Encoder& buffer )
	{
		::encode( iType, uiInstance, buffer );
	}
	
	virtual void decode( std::int32_t iType, std::uint32_t uiInstance, eg::Decoder& buffer )
	{
		::decode( iType, uiInstance, buffer );
	}
	
	bool receive( Event& event )
	{
		return m_pMegaProtocol->receive( event.data.type, event.data.instance, event.data.timestamp );
	}
	
	void send( const char* type, eg::TimeStamp timestamp, const void* value, std::size_t size )
	{
		m_pMegaProtocol->send( type, timestamp, value, size );
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
	return megastructure::g_pluginSymbol.receive( event );
}

void events::put( const char* type, eg::TimeStamp timestamp, const void* value, std::size_t size )
{
	megastructure::g_pluginSymbol.send( type, timestamp, value, size );
}
    
bool events::update()
{
    return true;
}

	)";
	os << szEventRoutines << "\n";
}

} //namespace megastructure