
#ifndef BUILDSYSTEM_TOOLS_15_SEPT_2020
#define BUILDSYSTEM_TOOLS_15_SEPT_2020

#include "eg_compiler/io/indexed_object.hpp"

#include <boost/timer/timer.hpp>
#include <boost/filesystem/path.hpp>

#include <optional>
#include <sstream>
#include <ostream>
#include <string>
#include <vector>
#include <set>
#include <memory>
#include <mutex>
#include <iomanip>

struct DiagnosticsConfig
{
    bool bShowMessages = true;
    bool bShowBenchmarks = true;
    mutable std::mutex mut;
};

struct Benchmark
{
private:
    std::ostream& os;
    const std::string msgBegin, msgEnd;
    boost::timer::cpu_timer timer_internal;
    const DiagnosticsConfig& config;
public:
    Benchmark( std::ostream& os, const DiagnosticsConfig& _config, const std::string& strBegin )
        :   os( os ),
            msgBegin( strBegin ),
            config( _config )
    {
        //if( config.bShowMessages )
        //{
        //    if( !msgBegin.empty() )
        //    {
        //        os << msgBegin;
        //    }
        //}
    }
    
    Benchmark( std::ostream& os, const DiagnosticsConfig& _config, const std::string& strBegin, const std::string& strEnd )
        :   os( os ),
            msgBegin( strBegin ),
            msgEnd( strEnd ),
            config( _config )
    {
        //if( config.bShowMessages )
        //{
        //    if( !msgBegin.empty() )
        //    {
        //        os << msgBegin;
        //        os.flush();
        //    }
        //}
    }
    
    ~Benchmark()
    {
        if( config.bShowBenchmarks )
        {
            std::lock_guard< std::mutex > lock( config.mut );
            if( !msgBegin.empty() )
            {
                os << timer_internal.format( 3, "%w" ) << ": " << msgBegin << std::endl;
            }
            else
            {
                os << timer_internal.format( 3, "%w" ) << std::endl;
            }
        }
    }
};

#define START_BENCHMARK_EXPLICIT_ENDMSG( outputStream, config, msgBegin, msgEnd )\
std::ostringstream _osBegin, _osEnd;\
_osBegin << std::setw( 30 ) << msgBegin;\
_isEnd << msgEnd;\
Benchmark _benchmark( outputStream, config, _osBegin.str(), msgEnd.str() );\

#define START_BENCHMARK_EXPLICIT( outputStream, config, msgBegin )\
std::ostringstream _osBegin, _osEnd;\
_osBegin << std::setw( 30 ) << msgBegin;\
Benchmark _benchmark( outputStream, config, _osBegin.str() );\

#define START_BENCHMARK( msgBegin )\
std::ostringstream _osBegin, _osEnd;\
_osBegin << std::setw( 30 ) << msgBegin;\
Benchmark _benchmark( std::cout, m_config, _osBegin.str() );\

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
    
    void invokeCachedCompiler( const Environment& environment, const Compilation& compilation );
    
    class Task
    {
    public:
        using Ptr = std::shared_ptr< Task >;
        using PtrVector = std::vector< Ptr >;
        using RawPtr = Task*;
        using RawPtrSet = std::set< RawPtr >;
        
        Task( const RawPtrSet& dependencies );
        
        virtual ~Task();
        
        virtual bool isReady( const RawPtrSet& finished );
        virtual void run() = 0;
        
    protected:
        RawPtrSet m_dependencies;
        
    };
    
    
    class Scheduler
    {
    public:
        static const int m_total_threads = 16;
    
        Scheduler( const Task::PtrVector& tasks );
        
        void run();
        
    private:
        void thread_run();
    
    private:
        Task::PtrVector m_tasks;
        Task::RawPtrSet m_pending, m_finished;
        std::mutex m_mutex;
    };
    
    
    
}












#endif //BUILDSYSTEM_TOOLS_15_SEPT_2020