
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


#ifndef MEGASTRUCTURE_GENERATOR_01_JULY_2020
#define MEGASTRUCTURE_GENERATOR_01_JULY_2020

#include <ostream>
#include <string>
#include <map>

#include "eg_compiler/layout.hpp"
#include "eg_compiler/translation_unit.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/dataAccessPrinter.hpp"
#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

class ProjectTree;
class HostName;

namespace megastructure
{

    class NetworkAnalysis
    {
    public:
        enum BufferRelation
        {
            eComponent,
            eProcess,
            ePlanet,
            TOTAL_RELATION_TYPES
        };
        
        using HashInt = std::int32_t;

        using HostNameBufferMap = std::map< const eg::Buffer*, std::shared_ptr< HostName > >;
        using BufferTypes = std::map< const eg::Buffer*, BufferRelation >;
        using DataTypeHashBases = std::map< const eg::DataMember*, HashInt >;
        
        struct HostStructures
        {
            std::string strWriteSetName, strIdentityEnumName;
            const eg::concrete::Action* pRoot;
        };
        using HostStructureMap = std::map< std::shared_ptr< HostName >, HostStructures >;
        
        NetworkAnalysis( const eg::ReadSession& session, const ProjectTree& project );
        
        inline std::size_t getReadBitSetSize() const { return m_hashTotal; }
            
        inline BufferRelation getBufferRelation( const eg::Buffer* pBuffer ) const
        {
            BufferTypes::const_iterator iFind = m_bufferTypes.find( pBuffer );
            VERIFY_RTE( iFind != m_bufferTypes.end() );
            return iFind->second;
        }
        
        bool isBufferInProcess( const eg::Buffer* pBuffer ) const
        {
            BufferTypes::const_iterator iFind = m_bufferTypes.find( pBuffer );
            VERIFY_RTE( iFind != m_bufferTypes.end() );
            switch( iFind->second )
            {
                case eComponent           : return true;
                case eProcess             : 
                    if( pBuffer->isSimple() )
                        return true;
                    else
                        return false;
                case ePlanet              : return false;
                case TOTAL_RELATION_TYPES :
                default:
                    THROW_RTE( "Unknown buffer type" );
                    
            }
            return iFind->second == eComponent;
        }
        
        HashInt getDataMemberReadHashBase( const eg::DataMember* pDataMember ) const
        {
            DataTypeHashBases::const_iterator iFind = m_hashBases.find( pDataMember );
            VERIFY_RTE( iFind != m_hashBases.end() );
            return iFind->second;
        }
            
        ::eg::PrinterFactory::Ptr getMegastructurePrinterFactory();
        
        const HostStructures& getHostStructures( std::shared_ptr< HostName > pHostName ) const
        {
            HostStructureMap::const_iterator iFind = m_hostStructures.find( pHostName );
            VERIFY_RTE( iFind != m_hostStructures.end() );
            return iFind->second;
        }
        const HostStructureMap& getHostStructures() const { return m_hostStructures; }
        
        const HostStructures& getHostStructures( const eg::Buffer* pBuffer ) const;
        
        const HostStructures* getHostStructures( const eg::concrete::Action* pAction ) const;
        
    private:
        void getBufferTypes();
        void getDataHashBases();
        void getHostStructures();

    private:
        const eg::ReadSession& m_session;
        const ProjectTree& m_project;
        HostNameBufferMap m_bufferHostNames;
        BufferTypes m_bufferTypes;
        DataTypeHashBases m_hashBases;
        HashInt m_hashTotal;
        HostStructureMap m_hostStructures;
    };

    enum ComponentType
    {
        eComponent_Basic,
        eComponent_Python,
        eComponent_Unreal,
        TOTAL_COMPONENT_TYPES
    };
    
    static const char* szComponentTypeNames[] = 
    {
        "basic",
        "python",
        "unreal"
    };

    void generate_eg_component( std::ostream& os, 
            const ProjectTree& project,
            const eg::ReadSession& session,
            const NetworkAnalysis& networkAnalysis,
            ComponentType componentType );

    class InstructionCodeGeneratorFactoryImpl : public eg::InstructionCodeGeneratorFactory
    {
    public:
        InstructionCodeGeneratorFactoryImpl( 
            const megastructure::NetworkAnalysis& networkAnalysis,
            const eg::TranslationUnitAnalysis& translationUnitAnalysis,
            const ProjectTree& projectTree );
        
        std::shared_ptr< eg::InstructionCodeGenerator > create( eg::CodeGenerator& generator, std::ostream& os );
        
    private:
        const megastructure::NetworkAnalysis& m_networkAnalysis;
        const eg::TranslationUnitAnalysis& m_translationUnitAnalysis;
        const ProjectTree& m_projectTree;
    };

}

#endif //MEGASTRUCTURE_GENERATOR_01_JULY_2020