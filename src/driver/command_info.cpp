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





#include "eg_compiler/sessions/implementation_session.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include <boost/program_options.hpp>
#include <boost/optional.hpp>

#include <iostream>

void command_info( bool bHelp, const std::vector< std::string >& args )
{
    std::string strDirectory;
    std::string strProject;
    
    bool bCode = false;
    bool bAbstract = false;
    bool bConcrete = false;
    bool bAll = false;
    
    namespace po = boost::program_options;
    po::options_description commandOptions(" Read Project Log");
    {
        commandOptions.add_options()
            ("dir",         po::value< std::string >( &strDirectory ),              "Project directory")
			("project", 	po::value< std::string >( &strProject ), "Project Name" )
            ("code",       po::bool_switch( &bCode ), "Print entire program as single eg file" )
            ("abstract",   po::bool_switch( &bAbstract ), "Print the abstract tree" )
            ("concrete",   po::bool_switch( &bConcrete ), "Print the concrete tree (default)" )
            ("all",        po::bool_switch( &bAll ), "Print all forms" )
        ;
    }
    
    po::positional_options_description p;
    p.add( "filters", -1 );
    
    po::variables_map vm;
    po::store( po::command_line_parser( args ).options( commandOptions ).positional( p ).run(), vm );
    po::notify( vm );
    
    if( bHelp )
    {
        std::cout << commandOptions << "\n";
    }
    else
    {
        if( !bCode && !bAbstract && !bConcrete )
            bAll = true;
        
        if( bAll )
        {
            bCode       = true;
            bAbstract   = true;
            bConcrete   = true;
        }
		
		if( strProject.empty() )
		{
            std::cout << "Missing project name" << std::endl;
            return;
		}
        
        const boost::filesystem::path projectDirectory = 
            boost::filesystem::edsCannonicalise(
                boost::filesystem::absolute( strDirectory ) );

        if( !boost::filesystem::exists( projectDirectory ) )
        {
            THROW_RTE( "Specified directory does not exist: " << projectDirectory.generic_string() );
        }
        else if( !boost::filesystem::is_directory( projectDirectory ) )
        {
            THROW_RTE( "Specified path is not a directory: " << projectDirectory.generic_string() );
        }
        
        Environment environment;
        
        ProjectTree projectTree( environment, projectDirectory, strProject );
        
        eg::ReadSession session( projectTree.getAnalysisFileName() );

        if( bCode )
        {
            std::cout << "//Code\n";
            const eg::interface::Root* pInterfaceRoot = session.getTreeRoot();
            std::string strIndent;
            pInterfaceRoot->print( std::cout, strIndent, true );
            std::cout << "\n\n\n";
        }
        
        if( bAbstract )
        {
            std::cout << "//Abstract Interface\n";
            const eg::interface::Root* pInterfaceRoot = session.getTreeRoot();
            std::string strIndent;
            pInterfaceRoot->print( std::cout, strIndent, false );
            std::cout << "\n\n\n";
        }
        
        if( bConcrete )
        {
            std::cout << "//Concrete Memory Model\n";
            const eg::concrete::Action* pConcreteRoot = session.getInstanceRoot();
            std::string strIndent;
            pConcreteRoot->print( std::cout, strIndent );
            std::cout << "\n\n\n";
        }
    }
}