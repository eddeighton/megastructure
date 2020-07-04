
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
        ePlanet
    };

    using BufferTypes = std::map< const eg::Buffer*, BufferRelation >;

    enum DataRelation
    {
        eNothing = -1,
        eSimLock = 0
    };

    using DataTypeHashBases = std::map< const eg::DataMember*, int >;
    
    
    struct HostStructures
    {
        std::string strWriteSetName, strActivationSetName, strIdentityEnumName;
    };
    using HostStructureMap = std::map< std::shared_ptr< HostName >, HostStructures >;
    

    NetworkAnalysis( const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, const ProjectTree& project );
    
    inline std::size_t getReadBitSetSize() const { return m_hashTotal; }
        
    inline bool isBufferForThisComponent( const eg::Buffer* pBuffer ) const
    {
        BufferTypes::const_iterator iFind = m_bufferTypes.find( pBuffer );
        VERIFY_RTE( iFind != m_bufferTypes.end() );
        return iFind->second == eComponent;
    }
    
    int getDataMemberReadHashBase( const eg::DataMember* pDataMember ) const
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
    
private:
    void getBufferTypes( const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
        const std::string& strCoordinator, const std::string& strHost );
        
    void getDataHashBases( const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
        const std::string& strCoordinator, const std::string& strHost );
        
    void getHostStructures( const ProjectTree& project );

private:
    const ProjectTree& m_project;
    BufferTypes m_bufferTypes;
    DataTypeHashBases m_hashBases;
    std::size_t m_hashTotal;
    HostStructureMap m_hostStructures;
};


void generate_eg_component( std::ostream& os, 
        const ProjectTree& project,
		const eg::ReadSession& session,
        const NetworkAnalysis& networkAnalysis );

}

#endif //MEGASTRUCTURE_GENERATOR_01_JULY_2020