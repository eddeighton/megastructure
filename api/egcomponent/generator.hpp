
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

namespace megastructure
{
    
enum BufferRelation
{
    eComponent,
    eProcess,
    ePlanet
};

using BufferTypes = std::map< const eg::Buffer*, BufferRelation >;

void getBufferTypes( const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
	const std::string& strCoordinator, const std::string& strHost, BufferTypes& bufferTypes );

::eg::PrinterFactory::Ptr getMegastructurePrinterFactory( 
    const eg::Layout& layout, const eg::TranslationUnitAnalysis& translationUnitAnalysis, 
        const std::string& strCoordinator, const std::string& strHost );

void generate_eg_component( std::ostream& os, 
        const ProjectTree& project,
		const eg::ReadSession& session );

}

#endif //MEGASTRUCTURE_GENERATOR_01_JULY_2020