
#include "build_tools.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <functional>
#include <thread>
#include <chrono>
#include <iostream>

namespace build
{
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Task::Task( const RawPtrSet& dependencies )
    :   m_dependencies( dependencies )
{
    
}
    
Task::~Task()
{
    
}
    
bool Task::isReady( const RawPtrSet& finished )
{
    for( RawPtr pTask : m_dependencies )
    {
        if( !finished.count( pTask ) )
            return false;
    }
    return true;
}
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
Scheduler::Scheduler( const Task::PtrVector& tasks )
    :   m_tasks( tasks )
{
    for( Task::Ptr pTask : m_tasks )
    {
        m_pending.insert( pTask.get() );
    }
}

void Scheduler::thread_run()
{
    bool bContinue = true;
    while( bContinue )
    {
        bool bRemainingTasks = false;
        Task::RawPtr pTaskTodo = nullptr;
        {
            std::lock_guard< std::mutex > lock( m_mutex );
            
            if( !m_pending.empty() )
            {
                bRemainingTasks = true;
                //attempt to find ready task
                for( Task::RawPtr pTask : m_pending )
                {
                    if( pTask->isReady( m_finished ) )
                    {
                        m_pending.erase( pTask );
                        pTaskTodo = pTask;
                        break;
                    }
                }
            }
        }
        
        if( bRemainingTasks )
        {
            if( pTaskTodo )
            {
                try
                {
                    pTaskTodo->run();
                }
                catch( std::exception& ex )
                {
                    std::cout << "ERROR in task: " << ex.what() << std::endl;
                    throw;
                }
                
                std::lock_guard< std::mutex > lock( m_mutex );
                m_finished.insert( pTaskTodo );
            }
            else
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for( 1ms );
            }
        }
        else
        {
            bContinue = false;
        }
    }
}

void Scheduler::run()
{
    std::vector< std::thread > threads;
    for( int i = 0; i < m_total_threads; ++i )
    {
        threads.push_back( 
            std::thread( 
                std::bind( &Scheduler::thread_run, std::ref( *this ) ) ) );
    }
    
    for( std::thread& th : threads )
    {
        if( th.joinable() )
            th.join();
    }
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void invokeCachedCompiler( const Environment& environment, const Compilation& compilation )
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
    
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

std::size_t hash( const std::vector< std::string >& strings )
{
    std::size_t szHash = 123456;
    
    for( const std::string& str : strings )
    {
        const std::size_t h = std::hash< std::string >{}( str );
        szHash = szHash ^ ( h << 1 );
    }
    
    return szHash;
}
    
struct Stash::Pimpl
{
    const Environment& m_environment;
    const boost::filesystem::path m_stashDirectory;
    
    using FileHash = std::pair< HashCode, boost::filesystem::path >;
    using Manifest = std::map< FileHash, boost::filesystem::path >;
    
    Manifest m_manifest;
    
    inline static const char* pszManifestFileName = "manifest.txt";
    
    static void load( std::istream& inStream, Manifest& output )
    {
        std::string strLine;
        while( std::getline( inStream, strLine ) )
        {
            using Tokeniser = boost::tokenizer< boost::char_separator< char > >;
            boost::char_separator< char > sep( "," );
            Tokeniser tokens( strLine, sep );
            for ( Tokeniser::iterator i = tokens.begin(); i != tokens.end(); ++i )
            {
                FileHash fileHash;
                fileHash.first = boost::lexical_cast< HashCode >( *i );
                
                if( ++i == tokens.end() )
                    THROW_RTE( "Error in stash manifest" );
                fileHash.second = *i;
                
                if( ++i == tokens.end() )
                    THROW_RTE( "Error in stash manifest" );
                
                output.insert( std::make_pair( fileHash, *i ) );
            }
        }
    }

    static void save( const Manifest& input, std::ostream& outStream )
    {
        for( Manifest::const_iterator i = input.begin(),
            iEnd = input.end(); i!=iEnd; ++i )
        {
            outStream << i->first.first << ',' << i->first.second.string() << ',' << i->second.string() << '\n';
        }
    }

    void loadManifest()
    {
        const boost::filesystem::path manifestFile = m_stashDirectory / pszManifestFileName;
        if( boost::filesystem::exists( manifestFile ) )
        {
            std::ifstream inputFileStream( manifestFile.native().c_str(), std::ios::in );
            if( !inputFileStream.good() )
            {
                THROW_RTE( "Failed to open file: " << manifestFile.string() );
            }
            load( inputFileStream, m_manifest );
        }
        else
        {
            m_manifest.clear();
        }
        
    }
    void saveManifest()
    {
        const boost::filesystem::path manifestFile = m_stashDirectory / pszManifestFileName;
        boost::filesystem::ensureFoldersExist( manifestFile );
        std::unique_ptr< boost::filesystem::ofstream > pFileStream = 
            boost::filesystem::createNewFileStream( manifestFile );
        save( m_manifest, *pFileStream );
    }
    
    std::mutex m_mutex;
    Pimpl( const Environment& environment, const boost::filesystem::path& stashDirectory )
        :   m_environment( environment ),
            m_stashDirectory( stashDirectory )
    {
        loadManifest();
    }
    /*~Pimpl()
    {
        saveManifest();
    }*/
    
    void stash( const boost::filesystem::path& file, const HashCode& code )
    {
        std::lock_guard lock( m_mutex );
        
        const boost::filesystem::path manifestFile = m_stashDirectory / pszManifestFileName;
        boost::filesystem::ensureFoldersExist( manifestFile );
        
        std::ostringstream osFileName;
        osFileName << "stash_" << m_manifest.size() << ".st";
        
        const boost::filesystem::path stashFile = m_stashDirectory / osFileName.str();

        if( boost::filesystem::exists( stashFile ) )
        {
            boost::filesystem::remove( stashFile );
        }
        boost::filesystem::copy( file, stashFile );
        
        m_manifest[ std::make_pair( code, file ) ] = stashFile;
        
        saveManifest();
    }

    bool restore( const boost::filesystem::path& file, const HashCode& code )
    {
        std::lock_guard lock( m_mutex );
        
        Manifest::const_iterator iFind = m_manifest.find( std::make_pair( code, file ) );
        if( iFind != m_manifest.end() )
        {
            boost::filesystem::path stashFile = iFind->second;
            
            if( boost::filesystem::exists( stashFile ) )
            {
                if( boost::filesystem::exists( file ) )
                {
                    boost::filesystem::remove( file );
                }
                //recheck the hash code??
                boost::filesystem::copy( stashFile, file );
                return true;
            }
        }
        return false;
    }
};

Stash::Stash( const Environment& environment, const boost::filesystem::path& stashDirectory )
    :   m_pPimpl( std::make_shared< Pimpl >( environment, stashDirectory ) )
{
    
}
    
void Stash::stash( const boost::filesystem::path& file, const HashCode& code )
{
    m_pPimpl->stash( file, code );
}

bool Stash::restore( const boost::filesystem::path& file, const HashCode& code )
{
    return m_pPimpl->restore( file, code );
}

}



