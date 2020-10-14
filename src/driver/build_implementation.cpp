
#include "build_implementation.hpp"

#include "megaxml/mega_schema.hxx"
#include "megaxml/mega_schema-pimpl.hxx"
#include "megaxml/mega_schema-simpl.hxx"

#include "common/assert_verify.hpp"
#include "common/file.hpp"

#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>

#pragma warning( push )
#pragma warning( disable : 4996) //iterator thing
#pragma warning( disable : 4244) //conversion to DWORD from system_clock::rep
#include <boost/process.hpp>
#pragma warning( pop )

#include <iostream>
#include <memory>
#include <map>


extern void generatePythonBindings( std::ostream&, const eg::ReadSession&, const megastructure::NetworkAnalysis&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateUnrealInterface( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateUnrealCode( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateGeometryInterface( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );
extern void generateGeometryCode( std::ostream&, const eg::ReadSession&, const Environment&, const ProjectTree&, eg::PrinterFactory::Ptr );

namespace build
{
namespace Implementation
{
static DiagnosticsConfig m_config;
    
void Task_CPPCompilation::run()
{
    m_taskInfo.taskName( "CPPCompilation" );
    m_taskInfo.source( m_sourceFile );
    m_taskInfo.target( m_projectTree.getObjectFile( m_sourceFile, m_binaryPath ) );
    updateProgress();
    
    std::size_t hashCode = task::hash_file( m_sourceFile );
    hashCode = task::hash_combine( hashCode, task::hash_strings( { m_strCompilationFlags } ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentIncludePCH( m_component ) ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentInterfacePCH( m_component ) ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentGenericsPCH( m_component ) ) );
    
    if( m_stash.restore( m_projectTree.getObjectFile( m_sourceFile, m_binaryPath ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
    build::Compilation cmd( m_sourceFile, 
                            m_projectTree.getObjectFile( m_sourceFile, m_binaryPath ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputOBJ{} );
    {
        cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
        cmd.inputPCH.push_back( m_projectTree.getComponentIncludePCH( m_component ) );
        cmd.inputPCH.push_back( m_projectTree.getComponentInterfacePCH( m_component ) );
        cmd.inputPCH.push_back( m_projectTree.getComponentGenericsPCH( m_component ) );
        cmd.includeDirs = m_projectTree.getComponentIncludeDirectories( m_environment, m_component );
        cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
        cmd.includeDirs.push_back( m_projectTree.getInterfaceFolder() );
        cmd.defines.push_back( m_strAdditionalDefines );
    }
    invokeCompiler( m_environment, cmd );     

    m_stash.stash( m_projectTree.getObjectFile( m_sourceFile, m_binaryPath ), hashCode );
    
    m_taskInfo.complete( true );    
}

void Task_PublicEGImplCompilation::run()
{
    const std::string strTUName = m_translationUnit.getName();
    
    m_taskInfo.taskName( "PublicEGImplCompilation" );
    m_taskInfo.source( m_projectTree.getImplementationSource( strTUName ) );
    m_taskInfo.target( m_projectTree.getObjectName( strTUName, m_binaryPath ) );
    updateProgress();
    
    std::vector< std::string > additionalIncludes = 
    { 
        m_projectTree.getStructuresInclude(), 
        m_projectTree.getNetStateSourceInclude() 
    };
    
    //generate the implementation source code
    std::ostringstream osImpl;
    eg::generateImplementationSource( osImpl, m_instructionCodeGenFactory, m_printerFactory, m_session, m_translationUnit, additionalIncludes );
    
    std::size_t hashCode = task::hash_strings( { osImpl.str(), m_strCompilationFlags } );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getInterfacePCH() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getGenericsPCH() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getOperationsPublicPCH( strTUName ) ) );
    
    if( m_stash.restore( m_projectTree.getObjectName( strTUName, m_binaryPath ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
    boost::filesystem::updateFileIfChanged( m_projectTree.getImplementationSource( strTUName ), osImpl.str() );
    
    build::Compilation cmd( m_projectTree.getImplementationSource( strTUName ), 
                            m_projectTree.getObjectName( strTUName, m_binaryPath ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputOBJ{} );
                            
    cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
    cmd.inputPCH.push_back( m_projectTree.getInterfacePCH() );
    cmd.inputPCH.push_back( m_projectTree.getGenericsPCH() );
    cmd.inputPCH.push_back( m_projectTree.getOperationsPublicPCH( strTUName ) );
    cmd.includeDirs = m_projectTree.getIncludeDirectories( m_environment );
    cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
    cmd.includeDirs.push_back( m_projectTree.getInterfaceFolder() );
    cmd.defines.push_back( m_strAdditionalDefines );
    
    invokeCompiler( m_environment, cmd );

    m_stash.stash( m_projectTree.getObjectName( strTUName, m_binaryPath ), hashCode );
    
    m_taskInfo.complete( true );        
}

void Task_PrivateEGImplCompilation::run()
{
    const std::string strTUName = m_translationUnit.getName();
    
    m_taskInfo.taskName( "PrivateEGImplCompilation" );
    m_taskInfo.source( m_projectTree.getImplementationSource( strTUName ) );
    m_taskInfo.target( m_projectTree.getObjectName( strTUName, m_binaryPath ) );
    updateProgress();
    
    std::vector< std::string > additionalIncludes = 
    { 
        m_projectTree.getStructuresInclude(), 
        m_projectTree.getNetStateSourceInclude() 
    };
    
    //generate the implementation source code
    std::ostringstream osImpl;
    eg::generateImplementationSource( osImpl, m_instructionCodeGenFactory, m_printerFactory, m_session, m_translationUnit, additionalIncludes );
    
    std::size_t hashCode = task::hash_strings( { osImpl.str(), m_strCompilationFlags } );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getParserDatabaseFilePreInterfaceAnalysis() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getIncludePCH() ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentIncludePCH( m_component ) ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentInterfacePCH( m_component ) ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getComponentGenericsPCH( m_component ) ) );
    hashCode = task::hash_combine( hashCode, m_stash.getHashCode( m_projectTree.getOperationsPrivatePCH( strTUName ) ) );
    
    if( m_stash.restore( m_projectTree.getObjectName( strTUName, m_binaryPath ), hashCode ) )
    {
        m_taskInfo.cached( true );
        m_taskInfo.complete( true );
        return;
    }
    
    boost::filesystem::updateFileIfChanged( m_projectTree.getImplementationSource( strTUName ), osImpl.str() );
    
    build::Compilation cmd( m_projectTree.getImplementationSource( strTUName), 
                            m_projectTree.getObjectName( strTUName, m_binaryPath ), 
                            m_strCompilationFlags, 
                            build::Compilation::OutputOBJ{} );
                            
    cmd.inputPCH.push_back( m_projectTree.getIncludePCH() );
    cmd.inputPCH.push_back( m_projectTree.getComponentIncludePCH( m_component ) );
    cmd.inputPCH.push_back( m_projectTree.getComponentInterfacePCH( m_component ) );
    cmd.inputPCH.push_back( m_projectTree.getComponentGenericsPCH( m_component ) );
    cmd.inputPCH.push_back( m_projectTree.getOperationsPrivatePCH( strTUName ) );
    cmd.includeDirs = m_projectTree.getComponentIncludeDirectories( m_environment, m_component );
    cmd.includeDirs.push_back( m_environment.getEGLibraryInclude() );
    cmd.includeDirs.push_back( m_projectTree.getInterfaceFolder() );
    cmd.defines.push_back( m_strAdditionalDefines );
    
    invokeCompiler( m_environment, cmd );

    m_stash.stash( m_projectTree.getObjectName( strTUName, m_binaryPath ), hashCode );   
    
    m_taskInfo.complete( true );     
}

void generateMegaBufferStructures( std::ostream& os, const eg::ReadSession& program, const ProjectTree& projectTree )
{
    const eg::Layout& layout = program.getLayout();
    
    eg::generateIncludeGuard( os, "STRUCTURES" );
    
    os << "//data structures\n";
    for( const eg::Buffer* pBuffer : layout.getBuffers() )
    {
        os << "\n//Buffer: " << pBuffer->getTypeName();
        if( const eg::concrete::Action* pAction = pBuffer->getAction() )
        {
            os << " type: " << pAction->getIndex();
        }
        
        os << /*" stride: " << pBuffer->getStride() <<*/ " size: " << pBuffer->getSize() << "\n";
        os << "struct " << pBuffer->getTypeName() << "\n{\n";
        for( const eg::DataMember* pDataMember : pBuffer->getDataMembers() )
        {
            os << "    ";
            eg::generateDataMemberType( os, pDataMember );
            os << " " << pDataMember->getName() << ";\n";
        }
        
        os << "};\n";
        os << "extern " << pBuffer->getTypeName() << " *" << pBuffer->getVariableName() << ";\n";
        
    }
    
    os << "\n" << eg::pszLine << eg::pszLine;
    os << "#endif\n";
}

void generateMegaStructureNetStateHeader( std::ostream& os, const eg::ReadSession& program, 
        const ProjectTree& projectTree, const megastructure::NetworkAnalysis& networkAnalysis )
{
    const eg::Layout& layout = program.getLayout();
    
	const std::string& strCoordinatorName   = projectTree.getCoordinatorName();
	const std::string& strHostName          = projectTree.getHostName();
	const std::string& strProjectName       = projectTree.getProjectName();
    
    eg::generateIncludeGuard( os, "NETSTATE" );
    os << "\n//network state data\n";
    
    os << "extern std::bitset< " << networkAnalysis.getReadBitSetSize() << " > g_reads;\n";
    
    const megastructure::NetworkAnalysis::HostStructureMap& hostStructures = 
        networkAnalysis.getHostStructures();
    
    //generate the host id enum
    os << "enum mega_HostID\n{\n";
    for( const auto& i : hostStructures )
    {
        os << "    " << i.second.strIdentityEnumName << "_read,\n";
        os << "    " << i.second.strIdentityEnumName << "_write,\n";
    }
    os << "    g_TotalHostLocks\n";
    os << "};\n";
    os << "extern std::bitset< g_TotalHostLocks > g_hostLocks;\n\n";
    
    os << "enum mega_HostCycleID\n{\n";
    for( const auto& i : hostStructures )
    {
        os << "    " << i.second.strIdentityEnumName << "_cycle,\n";
    }
    os << "    g_TotalHostCycles\n";
    os << "};\n";
    os << "extern std::array< eg::TimeStamp, g_TotalHostCycles > g_hostCycles;\n\n";
    
    //generate all externs
    for( const auto& i : hostStructures )
    {
        os << "extern std::set< eg::TypeInstance > " << i.second.strWriteSetName << ";\n";
    }
        
    os << "\n" << eg::pszLine << eg::pszLine;
    os << "#endif\n";
}

void generateMegaStructureClockImpl( std::ostream& os, const eg::ReadSession& session, 
        const ProjectTree& projectTree, const megastructure::NetworkAnalysis& networkAnalysis )
{
    
    const char szClockSrc[] = R"(
namespace
{

struct HostClock
{
public:
    typedef std::chrono::steady_clock ClockType;
    typedef ClockType::time_point Tick;
    typedef ClockType::duration TickDuration;
    typedef std::chrono::duration< float, std::ratio< 1 > > FloatTickDuration;
    
    HostClock()
    {
        m_lastTick = m_startTick = ClockType::now();
        m_cycle = 1U;
        m_ct    = m_dt = 0.0f;
    }
    
    inline void nextCycle()
    {
        const Tick nowTick = ClockType::now();
        m_dt = FloatTickDuration( nowTick - m_lastTick  ).count();
        m_ct = FloatTickDuration( nowTick - m_startTick ).count();
        m_lastTick = nowTick;
        ++m_cycle;
    }
    
    inline Tick actual()           const { return ClockType::now(); }
    inline eg::TimeStamp cycle()   const { return m_cycle; }
    inline float ct()              const { return m_ct; }
    inline float dt()              const { return m_dt; }
    
private:
    Tick m_lastTick, m_startTick;
    eg::TimeStamp m_cycle;
    float m_ct, m_dt;
} 

theClock;

}

float clock::ct()
{
    return theClock.ct();
}
float clock::dt()
{
    return theClock.dt();
}
void clock::next()
{
    theClock.nextCycle();
}
)";
    os << szClockSrc;

    os << "eg::TimeStamp clock::cycle( eg::TypeID type )\n";
    os << "{\n";
    
    const eg::IndexedObject::Array& objects = 
        session.getObjects( eg::IndexedObject::MASTER_FILE );
    std::vector< const eg::concrete::Action* > actions = 
        eg::many_cst< eg::concrete::Action >( objects );
        
    const megastructure::NetworkAnalysis::HostStructureMap& hostStructures = 
        networkAnalysis.getHostStructures();
    
    os << "    switch( type )\n";
    os << "    {\n";
    for( const eg::concrete::Action* pAction : actions )
    {
        if( const megastructure::NetworkAnalysis::HostStructures* pHost = 
            networkAnalysis.getHostStructures( pAction ) )
        {
    os << "        case " << pAction->getIndex() << ": return ";
        os << "eg::readlock< eg::TimeStamp, " << pHost->strIdentityEnumName << "_cycle , " << pHost->strIdentityEnumName << "_read, " << pHost->pRoot->getIndex() << 
            " >( g_hostCycles[ " << pHost->strIdentityEnumName << "_cycle ] );\n";
        }
    }
    os << "        default: return theClock.cycle();\n";
    os << "    }\n";
    os << "}\n";

}

void build_implementation( const boost::filesystem::path& projectDirectory, 
        const std::string& strCoordinator, 
        const std::string& strHost, 
        const std::string& strProject, 
        const std::string& strCompilationFlags,
        const std::vector< std::string >& inputSourceFileNameSet, 
        const boost::filesystem::path& binPath )
{
    Environment environment( projectDirectory );
            
    ProjectTree projectTree( environment, projectDirectory, strCoordinator, strHost, strProject );
    
    Coordinator::Ptr    pCoordinator;
    HostName::Ptr       pHostName;
    ProjectName::Ptr    pProjectName;
    
    for( Coordinator::Ptr pCoordinatorIter : projectTree.getCoordinators() )
    {
        if( strCoordinator == pCoordinatorIter->name() )
        {
            for( HostName::Ptr pHostNameIter : pCoordinatorIter->getHostNames() )
            {
                if( strHost == pHostNameIter->name() )
                {
                    for( ProjectName::Ptr pProjectNameIter : pHostNameIter->getProjectNames() )
                    {
                        if( strProject == pProjectNameIter->name() )
                        {
                            pCoordinator = pCoordinatorIter;
                            pHostName = pHostNameIter;
                            pProjectName = pProjectNameIter;
                            break;
                        }
                    }
                }
            }
        }
    }
    VERIFY_RTE( pCoordinator );
    VERIFY_RTE( pHostName );
    VERIFY_RTE( pProjectName );
    const Component component = { pCoordinator, pHostName, pProjectName };
    
    eg::ReadSession session( projectTree.getAnalysisFileName() );
				
	const eg::TranslationUnitAnalysis& translationUnits =
		session.getTranslationUnitAnalysis();
    
    megastructure::NetworkAnalysis networkAnalysis( session, projectTree );
    
    eg::PrinterFactory::Ptr pPrinterFactory = 
        networkAnalysis.getMegastructurePrinterFactory();
        
    megastructure::InstructionCodeGeneratorFactoryImpl 
        instructionCodeGenFactory( networkAnalysis, translationUnits, projectTree );
	
	std::vector< boost::filesystem::path > sourceFiles;
    
    sourceFiles.push_back( projectTree.getCoroutineFrameSourceFilePath( environment ) );
    sourceFiles.push_back( projectTree.getBasicSchedulerFilePath( environment ) );
	
	//generate the structures
	{
		std::ostringstream osStructures;
		generateMegaBufferStructures( osStructures, session, projectTree );
		boost::filesystem::updateFileIfChanged( projectTree.getDataStructureSource(), osStructures.str() );
	}
    
    //generate the netstate header
    {
		std::ostringstream osNetState;
		generateMegaStructureNetStateHeader( osNetState, session, projectTree, networkAnalysis );
		boost::filesystem::updateFileIfChanged( projectTree.getNetStateSource(), osNetState.str() );
    }
	
	//generate the runtime code
	{
		std::ostringstream osImpl;
		osImpl << "#include \"" << projectTree.getStructuresInclude() << "\"\n";
        osImpl << "#include \"" << projectTree.getNetStateSourceInclude() << "\"\n";
        
        eg::generateAccessorFunctionImpls( osImpl, *pPrinterFactory, session );
		eg::generateActionInstanceFunctions( osImpl, *pPrinterFactory, session );
        
        generateMegaStructureClockImpl( osImpl, session, projectTree, networkAnalysis );
        
		boost::filesystem::updateFileIfChanged( projectTree.getRuntimeSource(), osImpl.str() );
		sourceFiles.push_back( projectTree.getRuntimeSource() );
	}
    
    const Project& project = projectTree.getProject();
    const std::string& strProjectType = project.getHost().Type();
    
	//generate the eg component interface implementation
    if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Basic ] )
    {
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Basic );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Python ] )
    {
        //generate python bindings
		std::ostringstream osPython;
        generatePythonBindings( osPython, session, networkAnalysis, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getPythonSource(), osPython.str() );
		sourceFiles.push_back( projectTree.getPythonSource() );
        
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Python );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Unreal ] )
    {
        //generate the unreal interface
		std::ostringstream osUnrealInterface;
        generateUnrealInterface( osUnrealInterface, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getUnrealInterface(), osUnrealInterface.str() );
        
        //generate the unreal implementation
		std::ostringstream osUnreal;
        generateUnrealCode( osUnreal, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getUnrealSource(), osUnreal.str() );
		sourceFiles.push_back( projectTree.getUnrealSource() );
        
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Unreal );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else if( strProjectType == megastructure::szComponentTypeNames[ megastructure::eComponent_Geometry ] )
    {
        //generate the geometry implementation
		std::ostringstream osGeometry;
        generateGeometryCode( osGeometry, session, environment, projectTree, pPrinterFactory );
		boost::filesystem::updateFileIfChanged( projectTree.getGeometrySource(), osGeometry.str() );
		sourceFiles.push_back( projectTree.getGeometrySource() );
        
		std::ostringstream osEGComponent;
		megastructure::generate_eg_component( 
            osEGComponent, 
            projectTree, 
            session,
            networkAnalysis,
            megastructure::eComponent_Geometry );
		const boost::filesystem::path egComponentSourceFilePath = projectTree.getEGComponentSource();
		boost::filesystem::updateFileIfChanged( egComponentSourceFilePath, osEGComponent.str() );
		sourceFiles.push_back( egComponentSourceFilePath );
    }
    else
    {
        THROW_RTE( "Unknown project type: " << strProjectType );
    }
    
    std::string strAdditionalDefines;
    {
        std::ostringstream osDefines;
        osDefines << projectTree.getCoordinatorName() << '_' << projectTree.getHostName();
        strAdditionalDefines = osDefines.str();
    }
    
    task::Stash stash( projectTree.getStashFolder() );
    stash.loadHashCodes( projectTree.getBuildInfoFile() );
    
    BuildState buildState( environment, projectTree, m_config, strCompilationFlags, 
        strAdditionalDefines, binPath, stash, std::cout, session );
    
    task::Task::PtrVector tasks;
    {
        for( const boost::filesystem::path& sourceFilePath : sourceFiles )
        {
            Task_CPPCompilation* pTask = new Task_CPPCompilation( buildState, component, sourceFilePath );
            tasks.push_back( task::Task::Ptr( pTask ) );
        }
        
        const eg::TranslationUnitAnalysis& tuAnalysis = session.getTranslationUnitAnalysis();
        for( const boost::filesystem::path& egFilePath : inputSourceFileNameSet )
        {
            const eg::TranslationUnit* pTranslationUnit = nullptr;
            {
                for( const eg::TranslationUnit* pTUIter : tuAnalysis.getTranslationUnits() )
                {
                    if( egFilePath == pTUIter->getName() )
                    {
                        pTranslationUnit = pTUIter;
                        break;
                    }
                }
            }
            if( !pTranslationUnit )
            {
                THROW_RTE( "Task_EGImplCompilation: " << " COULD NOT LOCATE: " << environment.printPath( egFilePath ) );
                return;
            }
            
            if( ( pTranslationUnit->getCHD().pCoordinator->getIdentifier() == component.pCoordinator->name() ) &&
                ( pTranslationUnit->getCHD().pHostName->getIdentifier()    == component.pHostName->name() ) )
            {
                Task_PrivateEGImplCompilation* pTask = new Task_PrivateEGImplCompilation( 
                    buildState, 
                    component, 
                    egFilePath, 
                    instructionCodeGenFactory, 
                    *pPrinterFactory, 
                    *pTranslationUnit );
                tasks.push_back( task::Task::Ptr( pTask ) );
            }
            else
            {
                Task_PublicEGImplCompilation* pTask = new Task_PublicEGImplCompilation( 
                    buildState, 
                    component, 
                    egFilePath, 
                    instructionCodeGenFactory, 
                    *pPrinterFactory, 
                    *pTranslationUnit );
                tasks.push_back( task::Task::Ptr( pTask ) );
            }
            
            
        }
    }
    
    task::Scheduler scheduler( std::cout, tasks );
    
    scheduler.run();
    
    
}

}//namespace Implementation
}//namespace build


