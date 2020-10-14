
#ifndef BUILDSYSTEM_TOOLS_15_SEPT_2020
#define BUILDSYSTEM_TOOLS_15_SEPT_2020

#include "common/task.hpp"

#include "eg_compiler/io/indexed_object.hpp"

#include <boost/filesystem/path.hpp>

#include <optional>
#include <string>
#include <vector>


class Environment;

namespace build
{
    struct Compilation
    {
        std::string strCompilationFlags;
        boost::filesystem::path inputFile;
        std::vector< boost::filesystem::path > includeDirs;
        std::vector< boost::filesystem::path > inputPCH;
        
        std::optional< boost::filesystem::path > egdb;
        std::optional< boost::filesystem::path > egtu;
        std::optional< eg::IndexedObject::FileID > egtuid;
        
        std::optional< boost::filesystem::path > outputPCH;
        std::optional< boost::filesystem::path > outputObject;
        
        std::vector< std::string > defines;
        
        struct OutputPCH{};
        struct OutputOBJ{};
        
        Compilation( const boost::filesystem::path& _inputFile, 
                     const boost::filesystem::path& _outputFile, 
                     const std::string& _strCompilationFlags, 
                     OutputPCH )
            :   strCompilationFlags( _strCompilationFlags ),
                outputPCH( _outputFile ),
                inputFile( _inputFile )
        {
        }
        Compilation( const boost::filesystem::path& _inputFile, 
                     const boost::filesystem::path& _outputFile, 
                     const std::string& _strCompilationFlags, 
                     OutputOBJ )
            :   strCompilationFlags( _strCompilationFlags ),
                outputObject( _outputFile ),
                inputFile( _inputFile )
        {
        }
    };
    
    void invokeCompiler( const Environment& environment, const Compilation& compilation );
    
}












#endif //BUILDSYSTEM_TOOLS_15_SEPT_2020