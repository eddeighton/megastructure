
#include "build_tools.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "common/assert_verify.hpp"

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <sstream>

namespace build
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void invokeCompiler( const Environment& environment, const Compilation& compilation )
{
    std::ostringstream osCmd;
    
    environment.startCompilationCommand( osCmd );
    osCmd << " " << compilation.strCompilationFlags << " ";
    for( const std::string& strDefine : compilation.defines )
    {
        VERIFY_RTE( !strDefine.empty() );
        osCmd << "-D" << strDefine << " ";
    }
    
    //input pch
    for( const boost::filesystem::path& inputPCH : compilation.inputPCH )
    {
        osCmd << "-Xclang -include-pch ";
        osCmd << "-Xclang " << environment.printPath( inputPCH ) << " ";
    }
    
    //eg
    if( compilation.egdb )
    {
        osCmd << "-Xclang -egdb=" << environment.printPath( compilation.egdb.value() ) << " ";
        osCmd << "-Xclang -egdll=" << environment.printPath( environment.getClangPluginDll() ) << " ";
        
        if( compilation.egtu )
        {
            VERIFY_RTE( compilation.egtuid );
            osCmd << "-Xclang -egtu=" << environment.printPath( compilation.egtu.value() ) << " ";
            osCmd << "-Xclang -egtuid=" << compilation.egtuid.value() << " ";
        }
        else
        {
            VERIFY_RTE( !compilation.egtuid );
        }
    }
    else
    {
        VERIFY_RTE( !compilation.egtu );
        VERIFY_RTE( !compilation.egtuid );
    }
    
    //input
    osCmd << environment.printPath( compilation.inputFile ) << " ";
    
    //include directories
    for( const boost::filesystem::path& includeDir : compilation.includeDirs )
    {
        osCmd << "-I " << environment.printPath( includeDir ) << " ";
    }
    
    //output
    if( compilation.outputPCH )
    {
        VERIFY_RTE( !compilation.outputObject );
        osCmd << "-Xclang -emit-pch -o " << environment.printPath( compilation.outputPCH.value() ) << " ";
    }
    else if( compilation.outputObject )
    {
        VERIFY_RTE( !compilation.outputPCH );
        osCmd << "-c -o " << environment.printPath( compilation.outputObject.value() ) << " ";
    }
    else
    {
        THROW_RTE( "Missing compiler output" );
    }
    
    const int iResult = boost::process::system( osCmd.str() );
    if( iResult )
    {
        THROW_RTE( "Error invoking clang++ " << iResult );
    }
}

}



