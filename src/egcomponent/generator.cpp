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

void generate_eg_component( std::ostream& os, 
		const std::string& strProjectName, 
		const std::string& strCoordinator, 
		const std::string& strHost, 
		const eg::ReadSession& session )
{
	
	os << "//ed was here\n";
	os << "#include <iostream>\n";
	os << "#include <boost/fiber/all.hpp>\n";
	os << "#include <boost/config.hpp>\n";
	os << "\n";
	
	os << "#include \"egcomponent/egcomponent.hpp\"\n";
    os << "#include \"structures.hpp\"\n";
	
    const eg::Layout& layout = session.getLayout();
    const eg::IndexedObject::Array& objects = session.getObjects( eg::IndexedObject::MASTER_FILE );
	
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
                pBuffer->getVariableName() << "\" , " <<  pBuffer->getSize() * pBuffer->getStride() << " );\n";
        }
        else
        {
            os << "    " << pBuffer->getVariableName() << "_mega = pMemorySystem->getLocalBuffer( \"" << 
                pBuffer->getVariableName() << "\" , " <<  pBuffer->getSize() * pBuffer->getStride() << " );\n";
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
	
	
	const char szEventRoutines[] = R"(
	
	
eg::event_iterator events::getIterator()
{
    return 0;
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
    return false;
}

	)";
	os << szEventRoutines << "\n";

	
	const char szStuff[] = R"(
	
namespace megastructure
{
	
class EGComponentImpl : public EGComponent
{
	MemorySystem* m_pMemorySystem = nullptr;
	MegaProtocol* m_pMegaProtocol = nullptr;
public:
	virtual ~EGComponentImpl()
	{
	}
	
	virtual void Initialise( MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol )
	{
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
	
	virtual void Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance, std::string& buffer )
	{
	}
	virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer )
	{
	}
	virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer )
	{
	}
	virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance )
	{
	}
	virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance )
	{
	}
	virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance )
	{
	}
};

extern "C" BOOST_SYMBOL_EXPORT EGComponentImpl g_pluginSymbol;
EGComponentImpl g_pluginSymbol;

}
	
	)";
	
	os << szStuff << "\n";
	
}

