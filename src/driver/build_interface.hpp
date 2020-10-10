
#ifndef BUILD_INTERFACE_TASKS_16_SEPT_2020
#define BUILD_INTERFACE_TASKS_16_SEPT_2020

#include "build_tools.hpp"

#include "egcomponent/generator.hpp"

#include "schema/project.hpp"
#include "schema/projectTree.hpp"

#include "eg_compiler/allocator.hpp"
#include "eg_compiler/sessions/parser_session.hpp"
#include "eg_compiler/sessions/interface_session.hpp"
#include "eg_compiler/sessions/operations_session.hpp"
#include "eg_compiler/sessions/implementation_session.hpp"
#include "eg_compiler/codegen/codegen.hpp"
#include "eg_compiler/codegen/instructionCodeGenerator.hpp"

#include "boost/filesystem/path.hpp"

namespace build
{
namespace Interface
{

struct BuildState
{
    const Environment&          m_environment;
    const ProjectTree&          m_projectTree;
    const DiagnosticsConfig&    m_config;
    const std::string&          m_strCompilationFlags;
    build::Stash&               m_stash;
    bool                        m_parserChanged;
    
    mutable std::unique_ptr< eg::ParserSession >            m_session_parser;
    mutable std::unique_ptr< eg::InterfaceSession >         m_session_interface;
    mutable std::unique_ptr< eg::ImplementationSession >    m_session_implementation;
    
    BuildState( const Environment&          environment,
                const ProjectTree&          projectTree,
                const DiagnosticsConfig&    config,
                const std::string&          strCompilationFlags,
                build::Stash&               stash )
        :   m_environment         ( environment         ),
            m_projectTree         ( projectTree         ),
            m_config              ( config              ),
            m_strCompilationFlags ( strCompilationFlags ),
            m_stash               ( stash ),
            m_parserChanged       ( false )
    {
        
    }
};

class BaseTask : public build::Task
{
public:
    BaseTask( const BuildState& buildState, const RawPtrSet& dependencies )
        :   build::Task( dependencies ), 
            m_environment           ( buildState.m_environment              ),
            m_projectTree           ( buildState.m_projectTree              ),
            m_config                ( buildState.m_config                   ),
            m_stash                 ( buildState.m_stash                    ),
            m_parserChanged         ( buildState.m_parserChanged            ),
            m_strCompilationFlags   ( buildState.m_strCompilationFlags      ),
            m_session_parser        ( buildState.m_session_parser           ),
            m_session_interface     ( buildState.m_session_interface        ),
            m_session_implementation( buildState.m_session_implementation   )
    {
    }
    
protected:
    
    const Environment&          m_environment;
    const ProjectTree&          m_projectTree;
    const DiagnosticsConfig&    m_config;
    const std::string&          m_strCompilationFlags;
    build::Stash&               m_stash;
    const bool&                 m_parserChanged;
    
    std::unique_ptr< eg::ParserSession >&           m_session_parser;
    std::unique_ptr< eg::InterfaceSession >&        m_session_interface;
    std::unique_ptr< eg::ImplementationSession >&   m_session_implementation;
};


class Task_ParserSession : public BaseTask
{
public:
    Task_ParserSession( const BuildState& buildState )
        :   BaseTask( buildState, {} )
    {
    }
    virtual void run();
};

class Task_ParserSessionCopy : public BaseTask
{
    const Component m_component;
public:
    Task_ParserSessionCopy( const BuildState& buildState, Task_ParserSession* pDependency, const Component& component )
        :   BaseTask( buildState, { pDependency } ),
            m_component( component )
    {
    }
    virtual void run();
};

class Task_MainIncludePCH : public BaseTask
{
public:
    Task_MainIncludePCH( const BuildState& buildState, Task_ParserSession* pDependency )
        :   BaseTask( buildState, { pDependency } )
    {
    }
    virtual void run();
};

class Task_MainInterfacePCH : public BaseTask
{
public:
    Task_MainInterfacePCH( const BuildState& buildState, Task_MainIncludePCH* pDependency )
        :   BaseTask( buildState, { pDependency } )
    {
    }
    
    virtual void run();
};

class Task_InterfaceSession : public BaseTask
{
public:
    Task_InterfaceSession( const BuildState& buildState, Task_MainInterfacePCH* pDependency, const build::Task::RawPtrSet& copies )
        :   BaseTask( buildState, copies )
    {
        m_dependencies.insert( pDependency );
    }
    
    virtual void run();
};

class Task_MainGenericsPCH : public BaseTask
{
public:
    Task_MainGenericsPCH( const BuildState& buildState, Task_InterfaceSession* pDependency )
        :   BaseTask( buildState, { pDependency } )
    {
    }
    
    virtual void run();
};

class Task_ComponentIncludePCH : public BaseTask
{
    const Component m_component;
public:
    Task_ComponentIncludePCH( const BuildState& buildState, Task_MainIncludePCH* pDependency, const Component& component )
        :   BaseTask( buildState, { pDependency } ),
            m_component( component )
    {
    }
    
    virtual void run();
};

class Task_ComponentInterfacePCH : public BaseTask
{
    const Component m_component;
public:
    Task_ComponentInterfacePCH( const BuildState& buildState, 
        Task_ComponentIncludePCH* pDependency, 
        Task_ParserSessionCopy* pDependency2, 
        Task_MainInterfacePCH* pDependency3, 
        const Component& component )
        :   BaseTask( buildState, { pDependency, pDependency2, pDependency3 } ),
            m_component( component )
    {
    }
    
    virtual void run();
};

class Task_ComponentGenericsPCH : public BaseTask
{
    const Component m_component;
public:
    Task_ComponentGenericsPCH( const BuildState& buildState, 
                Task_ComponentInterfacePCH* pDependency, 
                Task_InterfaceSession* pDependency2, 
                Task_MainGenericsPCH* pDependency3,
                const Component& component )
        :   BaseTask( buildState, { pDependency, pDependency2, pDependency3 } ),
            m_component( component )
    {
    }
    
    virtual void run();
};

class Task_OperationsHeader : public BaseTask
{
    const boost::filesystem::path m_sourceFile;
public:
    Task_OperationsHeader( const BuildState& buildState, Task_InterfaceSession* pDependency, const boost::filesystem::path& sourceFile )
        :   BaseTask( buildState, { pDependency } ),
            m_sourceFile( sourceFile )
    {
    }
    
    virtual void run();
};

class Task_OperationsPublicPCH : public BaseTask
{
    const boost::filesystem::path m_sourceFile;
public:
    Task_OperationsPublicPCH( const BuildState& buildState, Task_MainGenericsPCH* pDependency, Task_OperationsHeader* pDependency2, const boost::filesystem::path& sourceFile )
        :   BaseTask( buildState, { pDependency, pDependency2 } ),
            m_sourceFile( sourceFile )
    {
    }
    
    virtual void run();
};

class Task_OperationsPrivatePCH : public BaseTask
{
    const Component m_component;
    const boost::filesystem::path m_sourceFile;
public:
    Task_OperationsPrivatePCH( const BuildState& buildState, 
                Task_ComponentGenericsPCH* pDependency, 
                Task_OperationsPublicPCH* pDependency2, 
                const Component& component, 
                const boost::filesystem::path& sourceFile )
        :   BaseTask( buildState, { pDependency, pDependency2 } ),
            m_component( component ),
            m_sourceFile( sourceFile )
    {
    }
    
    virtual void run();
};


class Task_ImplementationSession : public BaseTask
{
public:
    Task_ImplementationSession( const BuildState& buildState, const build::Task::RawPtrSet& dependencies )
        :   BaseTask( buildState, dependencies )
    {
    }
    
    virtual void run();
};

}
}



#endif //BUILD_INTERFACE_TASKS_16_SEPT_2020