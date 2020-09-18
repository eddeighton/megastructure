

#ifndef BUILD_IMPLEMENTATION_TASKS_16_SEPT_2020
#define BUILD_IMPLEMENTATION_TASKS_16_SEPT_2020

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

struct BuildState
{
    const Environment&              m_environment;
    const ProjectTree&              m_projectTree;
    const DiagnosticsConfig&        m_config;
    const std::string&              m_strCompilationFlags;
    const std::string&              m_strAdditionalDefines;
    const boost::filesystem::path&  m_binaryPath;
    build::Stash&                   m_stash;
    const eg::ReadSession&          m_session;
    
    BuildState( const Environment&              environment,
                const ProjectTree&              projectTree,
                const DiagnosticsConfig&        config,
                const std::string&              strCompilationFlags,
                const std::string&              strAdditionalDefines,
                const boost::filesystem::path&  binaryPath,
                build::Stash&                   stash,
                const eg::ReadSession&          session  )
        :   m_environment          ( environment            ),
            m_projectTree          ( projectTree            ),
            m_config               ( config                 ),
            m_strCompilationFlags  ( strCompilationFlags    ),
            m_strAdditionalDefines ( strAdditionalDefines   ),
            m_binaryPath           ( binaryPath             ),
            m_stash                ( stash                  ),
            m_session              ( session                )
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
            m_strCompilationFlags   ( buildState.m_strCompilationFlags      ),
            m_strAdditionalDefines  ( buildState.m_strAdditionalDefines     ),
            m_binaryPath            ( buildState.m_binaryPath               ),
            m_stash                 ( buildState.m_stash                    ),
            m_session               ( buildState.m_session                  )
    {
    }
    
protected:
    const Environment&              m_environment;
    const ProjectTree&              m_projectTree;
    const DiagnosticsConfig&        m_config;
    const std::string&              m_strCompilationFlags;
    const std::string&              m_strAdditionalDefines;
    const boost::filesystem::path&  m_binaryPath;
    build::Stash&                   m_stash;
    const eg::ReadSession&          m_session;
};


class Task_CPPCompilation : public BaseTask
{
    const Component m_component;
    const boost::filesystem::path m_sourceFile;
public:
    Task_CPPCompilation( const BuildState& buildState, const Component& component, const boost::filesystem::path& sourceFile )
        :   BaseTask( buildState, {} ),
            m_component( component ),
            m_sourceFile( sourceFile )
    {
    }
    
    virtual void run();
};

class Task_PublicEGImplCompilation : public BaseTask
{
    const Component m_component;
    const boost::filesystem::path m_sourceFile;
    megastructure::InstructionCodeGeneratorFactoryImpl& m_instructionCodeGenFactory;
    eg::PrinterFactory& m_printerFactory;
    const eg::TranslationUnit& m_translationUnit;
public:
    Task_PublicEGImplCompilation( const BuildState& buildState, 
            const Component& component, 
            const boost::filesystem::path& sourceFile,
            megastructure::InstructionCodeGeneratorFactoryImpl& instructionCodeGenFactory,
            eg::PrinterFactory& printerFactory,
            const eg::TranslationUnit& translationUnit
    )
        :   BaseTask( buildState, {} ),
            m_component( component ),
            m_sourceFile( sourceFile ),
            m_instructionCodeGenFactory( instructionCodeGenFactory ),
            m_printerFactory( printerFactory ),
            m_translationUnit( translationUnit )
    {
    }
    
    virtual void run();
};
class Task_PrivateEGImplCompilation : public BaseTask
{
    const Component m_component;
    const boost::filesystem::path m_sourceFile;
    megastructure::InstructionCodeGeneratorFactoryImpl& m_instructionCodeGenFactory;
    eg::PrinterFactory& m_printerFactory;
    const eg::TranslationUnit& m_translationUnit;
public:
    Task_PrivateEGImplCompilation( const BuildState& buildState, 
            const Component& component, 
            const boost::filesystem::path& sourceFile,
            megastructure::InstructionCodeGeneratorFactoryImpl& instructionCodeGenFactory,
            eg::PrinterFactory& printerFactory,
            const eg::TranslationUnit& translationUnit
    )
        :   BaseTask( buildState, {} ),
            m_component( component ),
            m_sourceFile( sourceFile ),
            m_instructionCodeGenFactory( instructionCodeGenFactory ),
            m_printerFactory( printerFactory ),
            m_translationUnit( translationUnit )
    {
    }
    
    virtual void run();
};

#endif //BUILD_IMPLEMENTATION_TASKS_16_SEPT_2020