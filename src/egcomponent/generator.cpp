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


#include <ostream>
#include <iostream>

#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/translation_unit.hpp"

enum BufferRelation
{
    eComponent,
    eProcess,
    ePlanet
};

BufferRelation getBufferRelation( const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
	const std::string& strCoordinator, const std::string& strHost, const eg::Buffer* pBuffer )
{
	const eg::concrete::Action* pConcreteAction = pBuffer->getAction();
    const eg::interface::Action* pInterfaceAction = pConcreteAction->getAction();
	const eg::TranslationUnit* pUnit = translationUnitAnalysis.getActionTU( pInterfaceAction );
		
	if( pUnit->getCoordinatorHostnameDefinitionFile().isCoordinator( strCoordinator ) )
    {
        if( pUnit->getCoordinatorHostnameDefinitionFile().isHost( strHost ) )
        {
            return eComponent;
        }
        else
        {
            return eProcess;
        }
    }
    else
    {
        return ePlanet;
    }
}

void recurseEncodeDecode( const ::eg::concrete::Action* pAction, std::ostream& os )
{
	const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pNestedAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			const std::string strType = pNestedAction->getAction()->getStaticType();
			os << "    template<> inline void encode( Encoder& encoder, const " << strType << "& value ){ encode( encoder, value.data ); }\n";
			os << "    template<> inline void decode( Decoder& decoder, " << strType << "& value ) 		{ decode( decoder, value.data ); }\n";

			recurseEncodeDecode( pNestedAction, os );
		}
	}
}

void recurseEncode( const eg::Layout& layout, const ::eg::concrete::Action* pAction, std::ostream& os )
{
	os << "        case " << pAction->getIndex() << ": ";
	pAction->printEncode( os, "uiInstance" );
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
			pDataMember->printEncode( os, "uiInstance" );
			os << " break; //" << pDataMember->getName() << "\n";
		}
	}
}

void recurseDecode( const eg::Layout& layout, const ::eg::concrete::Action* pAction, std::ostream& os )
{
	os << "        case " << pAction->getIndex() << ": ";
	pAction->printDecode( os, "uiInstance" );
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
			pDataMember->printDecode( os, "uiInstance" );
			os << " break; //" << pDataMember->getName() << "\n";
		}
	}
}

void generate_eg_component( std::ostream& os, 
		const std::string& strProjectName, 
		const std::string& strCoordinator, 
		const std::string& strHost, 
		const eg::ReadSession& session )
{
	
	os << "//ed was here\n";
	os << "#include <iostream>\n";
	os << "#include <chrono>\n";
	os << "#include <thread>\n";
	os << "#include <vector>\n";
	os << "#include \"boost/fiber/all.hpp\"\n";
	os << "#include \"boost/config.hpp\"\n";

	os << "\n";
	
	os << "#include \"egcomponent/egcomponent.hpp\"\n";
    os << "#include \"egcomponent/traits.hpp\"\n";
    os << "#include \"structures.hpp\"\n";
	
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
	const eg::concrete::Action* pRoot = session.getInstanceRoot();
	
    const eg::TranslationUnitAnalysis& translationUnitAnalysis =
		session.getTranslationUnitAnalysis();
	
    os << "\n//buffers\n";
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
		auto r  = getBufferRelation( translationUnitAnalysis, strCoordinator, strHost, pBuffer );
		std::cout << "Buffer: " << pBuffer->getVariableName() << " relation: " << r << std::endl;
        if( r == eComponent )
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
        if( getBufferRelation( translationUnitAnalysis, strCoordinator, strHost, pBuffer ) == eComponent )
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
        for( const eg::DataMember* pDimension : pBuffer->getDimensions() )
        {
            pDimension->printAllocation( os, "i" );
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
        for( const eg::DataMember* pDimension : pBuffer->getDimensions() )
        {
            pDimension->printDeallocation( os, "i" );
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
		
		boost::fibers::use_scheduling_algorithm< eg::eg_algorithm >();
		
		m_pMemorySystem = pMemorySystem;
		m_pMegaProtocol = pMegaProtocol;
		
		allocate_buffers( m_pMemorySystem );
		
        boost::this_fiber::properties< eg::fiber_props >().setTimeKeeper();
	}
	virtual void Uninitialise()
	{
		deallocate_buffers( m_pMemorySystem );
	}
	
	virtual void Cycle()
	{
		eg::wait();
		clock::next();
	}
	
	virtual void WaitForReadResponse( std::int32_t iType, std::uint32_t uiInstance )
	{
        bool bCompleted = false;
		{
			boost::fibers::fiber executionFiber
			(
				std::allocator_arg,
				boost::fibers::fixedsize_stack( 4096 ),
				[ iType, uiInstance, &bCompleted ]()
				{
					boost::this_fiber::properties< eg::fiber_props >().setResumption( 
						[ iType, uiInstance ]( Event e )
						{
							return ( iType 		== e.data.type && 
									 uiInstance == e.data.instance );
						}
					);
					boost::this_fiber::yield();
					bCompleted = true;
				}
			);
			executionFiber.properties< eg::fiber_props >().setReference( eg::reference{ uiInstance, iType, 0 } );
			executionFiber.detach();
		}
		
        while( !bCompleted )
        {
			//spin the simulation while waiting for response
		    eg::wait();
		    clock::next();
			
			//could sleep here??
			using namespace std::chrono_literals;
			std::this_thread::sleep_for( 1ms );
        }
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

