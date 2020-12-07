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

#include "build_tools.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "eg_compiler/allocator.hpp"
#include "eg_compiler/sessions/parser_session.hpp"
#include "eg_compiler/sessions/interface_session.hpp"
#include "eg_compiler/sessions/operations_session.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <boost/timer/timer.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <iostream>
#include <memory>
#include <map>

namespace build
{
    namespace Interface
    {
        extern void build_interface( const boost::filesystem::path& projectDirectory, const std::string& strProject, const std::string& strCompilationFlags );
    }

    namespace Implementation
    {
        extern void build_implementation( const boost::filesystem::path& projectDirectory,
                const std::string& strCoordinator,
                const std::string& strHost,
                const std::string& strProject,
                const std::string& strCompilationFlags,
                const std::vector< std::string >& egSourceFiles,
                const boost::filesystem::path& binPath );
    }
}

void command_build( bool bHelp, const std::string& strBuildCommand, const std::vector< std::string >& args )
{
    std::string strDirectory, strProject, strCoordinator, strHost, strBin;

    boost::filesystem::path binPath;
    //bool bBenchCommands = false;
    //bool bLogCommands = false;
    bool bFullRebuild = false;
    std::vector< std::string > flags;
    std::string names;

    namespace po = boost::program_options;
    po::options_description commandOptions(" Build Project Command");
    {
        commandOptions.add_options()
            ("dir",         po::value< std::string >( &strDirectory ), "Project directory")
            ("project",     po::value< std::string >( &strProject ), "Project Name" )
            ("coordinator", po::value< std::string >( &strCoordinator ), "Coordinator Name" )
            ("host",        po::value< std::string >( &strHost ), "Host Name" )
            ("bin",         po::value< boost::filesystem::path >( &binPath ), "Binary Directory" )
            //("bench",     po::bool_switch( &bBenchCommands ), "Benchmark compilation steps" )
            //("trace",     po::bool_switch( &bLogCommands ), "Trace compilation commands" )
            ("full",        po::bool_switch( &bFullRebuild ), "Full rebuild - do not reuse previous objects or precompiled headers" )
            ("flags",       po::value< std::vector< std::string > >( &flags ), "C++ Compilation Flags" )
            ("names",       po::value< std::string >( &names ), "eg source file names ( no extension, semicolon delimited )" )
        ;
    }

    po::positional_options_description p;
    p.add( "dir", -1 );

    po::variables_map vm;
    po::store( po::command_line_parser( args ).options( commandOptions ).positional( p ).run(), vm );
    po::notify( vm );

    if( bHelp )
    {
        std::cout << commandOptions << "\n";
    }
    else
    {
        if( strBuildCommand.empty() )
        {
            std::cout << "Missing build command type" << std::endl;
            return;
        }

        std::string strCompilationFlags;
        {
            std::ostringstream osFlags;
            for( const auto& str : flags )
                osFlags << str << " ";
            strCompilationFlags = osFlags.str();
        }

        if( strCompilationFlags.empty() )
        {
            //use defaults
            std::cout << "Warning using default compiler flags as none specified" << std::endl;
            strCompilationFlags = "-O3 -mllvm -polly -MD -DNDEBUG -D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH -DWIN32_LEAN_AND_MEAN -D_DLL -DNOMINMAX -DBOOST_ALL_NO_LIB -D_CRT_SECURE_NO_WARNINGS -DBOOST_USE_WINDOWS_H -D_WIN32_WINNT=0x0601 -DWIN32 -D_WINDOWS -Xclang -std=c++17 -Xclang -fcoroutines-ts -Xclang -flto-visibility-public-std -Xclang -Wno-deprecated -Xclang -fexceptions -Xclang -Wno-inconsistent-missing-override";
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

        if( bFullRebuild )
        {
            Environment environment;
            ProjectTree projectTree( environment, projectDirectory, strProject );
            if( boost::filesystem::exists( projectTree.getInterfaceFolder() ) )
            {
                std::cout << "Removing: " << projectTree.getInterfaceFolder().generic_string() << std::endl;
                boost::filesystem::remove_all( projectTree.getInterfaceFolder() );
            }
            if( boost::filesystem::exists( projectTree.getImplFolder() ) )
            {
                std::cout << "Removing: " << projectTree.getImplFolder().generic_string() << std::endl;
                boost::filesystem::remove_all( projectTree.getImplFolder() );
            }
            if( boost::filesystem::exists( projectTree.getStashFolder() ) )
            {
                std::cout << "Removing: " << projectTree.getStashFolder().generic_string() << std::endl;
                boost::filesystem::remove_all( projectTree.getStashFolder() );
            }
        }

        if( strCoordinator.empty() && strHost.empty() )
        {
            build::Interface::build_interface( projectDirectory, strProject, strCompilationFlags );
        }
        else
        {
            //tokenize semi colon delimited names
            std::vector< std::string > egFileNames;
            {
                using Tokeniser = boost::tokenizer< boost::char_separator< char > >;
                boost::char_separator< char > sep( ";" );
                Tokeniser tokens( names, sep );
                for ( Tokeniser::iterator i = tokens.begin(); i != tokens.end(); ++i )
                    egFileNames.push_back( *i );
            }
            //for( const std::string& str : egFileNames )
            //{
            //    std::cout << "eg source name: " << str << std::endl;
            //}

            VERIFY_RTE_MSG( !strCoordinator.empty(), "Missing Coordinator" );
            VERIFY_RTE_MSG( !strHost.empty(), "Missing Host Name" );

            build::Implementation::build_implementation(
                projectDirectory, strCoordinator, strHost, strProject, strCompilationFlags, egFileNames, binPath );
        }
    }

}
