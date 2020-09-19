
#ifndef BUILDSYSTEM_TOOLS_15_SEPT_2020
#define BUILDSYSTEM_TOOLS_15_SEPT_2020

#include "eg_compiler/io/indexed_object.hpp"

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
    
    class TaskInfo
    {
        
        std::string m_strTaskName, m_strSource, m_strTarget;
        bool m_bCached = false, m_bComplete = false;
        std::vector< std::string > m_msgs;
        struct TaskInfoPimpl;
        std::shared_ptr< TaskInfoPimpl > m_pPimpl;
        mutable std::mutex m_mutex;
    public:
        using Ptr = std::shared_ptr< TaskInfo >;
        using PtrVector = std::vector< Ptr >;
        
        TaskInfo();
        
        void update();
        
        const std::string& taskName() const             { std::lock_guard< std::mutex > lock( m_mutex ); return m_strTaskName;         }
        const std::string& source() const               { std::lock_guard< std::mutex > lock( m_mutex ); return m_strSource;           }
        const std::string& target() const               { std::lock_guard< std::mutex > lock( m_mutex ); return m_strTarget;           }
        const bool cached() const                       { std::lock_guard< std::mutex > lock( m_mutex ); return m_bCached;             }
        const bool complete() const                     { std::lock_guard< std::mutex > lock( m_mutex ); return m_bComplete;           }
        const std::vector< std::string >& msgs() const  { std::lock_guard< std::mutex > lock( m_mutex ); return m_msgs;                }
        
        void taskName( const std::string& strTaskName )         { std::lock_guard< std::mutex > lock( m_mutex ); m_strTaskName = strTaskName;   }
        void source( const std::string& strSource )             { std::lock_guard< std::mutex > lock( m_mutex ); m_strSource = strSource;       }
        void source( const boost::filesystem::path& file )      { std::lock_guard< std::mutex > lock( m_mutex ); m_strSource = file.string();   }
        void target( const std::string& strTarget )             { std::lock_guard< std::mutex > lock( m_mutex ); m_strTarget = strTarget;       }
        void target( const boost::filesystem::path& file )      { std::lock_guard< std::mutex > lock( m_mutex ); m_strTarget = file.string();   }
        void cached( bool bCached )                             { std::lock_guard< std::mutex > lock( m_mutex ); m_bCached = bCached;           }
        void complete( bool bComplete )                         { std::lock_guard< std::mutex > lock( m_mutex ); m_bComplete = bComplete;       }
        void msg( const std::string& strMsg )                   { std::lock_guard< std::mutex > lock( m_mutex ); m_msgs.push_back( strMsg );    }
    };
    
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
        
        TaskInfo& getTaskInfo() { return m_taskInfo; }
        
        void updateProgress() { m_taskInfo.update(); }
        
    protected:
        RawPtrSet m_dependencies;
        TaskInfo m_taskInfo;
    };
    
    class Scheduler
    {
    public:
        static const int m_total_threads = 11;
    
        Scheduler( const Task::PtrVector& tasks );
        
        void run();
        
    private:
        void thread_run();
    
    private:
        Task::PtrVector m_tasks;
        Task::RawPtrSet m_pending, m_finished;
        std::mutex m_mutex;
    };
    
    using HashCode = std::size_t;
    HashCode hash_strings( const std::vector< std::string >& strings );
    HashCode hash_file( const boost::filesystem::path& file );
    HashCode hash_combine( HashCode left, HashCode right );
    
    class Stash
    {
    public:
        Stash( const Environment& environment, const boost::filesystem::path& stashDirectory );
        
        HashCode getHashCode( const boost::filesystem::path& key ) const;
        void setHashCode( const boost::filesystem::path& key, HashCode hashCode );
        void loadHashCodes( const boost::filesystem::path& file );
        void saveHashCodes( const boost::filesystem::path& file ) const;
        
        void stash( const boost::filesystem::path& file, const HashCode& code );
        bool restore( const boost::filesystem::path& file, const HashCode& code );
    private:
        struct Pimpl;
        std::shared_ptr< Pimpl > m_pPimpl;
    };
    
}












#endif //BUILDSYSTEM_TOOLS_15_SEPT_2020