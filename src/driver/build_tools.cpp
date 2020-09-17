
#include "build_tools.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

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
    

}



