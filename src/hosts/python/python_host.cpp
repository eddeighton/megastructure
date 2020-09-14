
#include <cstdio>
#include <memory>

#include <pybind11/pybind11.h>

#include "megastructure/component.hpp"
#include "megastructure/program.hpp"

namespace pybind11
{
    namespace detail
    {
        template <> struct type_caster< boost::filesystem::path >
        {
        public:
            PYBIND11_TYPE_CASTER( boost::filesystem::path, _("path"));
        
            bool load( handle src, bool )
            {
                PyObject* pSourcePythonObject = src.ptr();
                
                if( !PyUnicode_Check( pSourcePythonObject ) )
                    return false;
                
                value = boost::filesystem::path( PyUnicode_AS_DATA( pSourcePythonObject ) );
                
                return !PyErr_Occurred();
            }
        
            static handle cast( boost::filesystem::path src, return_value_policy /* policy */, handle /* parent */)
            {
                return PyUnicode_FromString( src.string().c_str() );
            }
        };
    }
}

namespace megastructure
{
    
class ProgramWrapper
{
public:
    ProgramWrapper( std::shared_ptr< Program > pProgram )
        : m_pProgram( pProgram )
    {
        
    }

    const std::string& getHostName() const
    {
        if( std::shared_ptr< Program > pProgram = m_pProgram.lock() )
        {
            return pProgram->getHostName();
        }
        else
        {
            return m_dummyString;
        }
    }
    
    const std::string& getProjectName() const
    {
        if( std::shared_ptr< Program > pProgram = m_pProgram.lock() )
        {
            return pProgram->getProjectName();
        }
        else
        {
            return m_dummyString;
        }
    }
    const std::string& getComponentName() const
    {
        if( std::shared_ptr< Program > pProgram = m_pProgram.lock() )
        {
            return pProgram->getComponentName();
        }
        else
        {
            return m_dummyString;
        }
    }
    
    const boost::filesystem::path& getComponentPath() const
    {
        if( std::shared_ptr< Program > pProgram = m_pProgram.lock() )
        {
            return pProgram->getComponentPath();
        }
        else
        {
            return m_dummyPath;
        }
    }
    
    void* getRoot() const
    {
        if( std::shared_ptr< Program > pProgram = m_pProgram.lock() )
        {
            return pProgram->getRoot();
        }
        else
        {
            return nullptr;
        }
    }
    

private:
    std::weak_ptr< Program > m_pProgram;
    std::string m_dummyString;
    boost::filesystem::path m_dummyPath;
};

class Host
{
    Host( const Host& ) = delete;
    Host& operator=( const Host& ) = delete;
public:
    Host( const std::string& strProjectDirectory, 
        const std::string& strMegaPort, 
        const std::string& strEGPort, 
        const std::string& strHostProgramName )
        :   m_environment( strProjectDirectory ),
            m_component( m_environment, strMegaPort, strEGPort, strHostProgramName, nullptr )
    {
        
    }
    
    const std::string& getHostProgramName()    const { return m_component.getHostProgramName(); }
    const std::string& getSlaveName()          const { return m_component.getSlaveName(); }
    const boost::filesystem::path& getSlaveWorkspacePath()  const { return m_component.getSlaveWorkspacePath(); }

    std::shared_ptr< ProgramWrapper > getProgram() 
    { 
        return std::make_shared< ProgramWrapper >( m_component.getProgram() ); 
    }
    
    void runCycle() { m_component.runCycle(); }
    
private:
    Environment m_environment;
    megastructure::Component m_component;
  
};

std::shared_ptr< Host > g_theHost;

std::shared_ptr< Host > NewHost( const std::string& strProjectDirectory, 
                                        const std::string& strMegaPort, 
                                        const std::string& strEGPort, 
                                        const std::string& strHostProgramName )
{
    g_theHost.reset();
    g_theHost = std::make_shared< Host >( strProjectDirectory, strMegaPort, strEGPort, strHostProgramName );
    return g_theHost;
}

std::shared_ptr< Host > GetHost()
{
    return g_theHost;
}

}

#ifdef _DEBUG
PYBIND11_MODULE( python_hostd, phModule ) 
#else
PYBIND11_MODULE( python_host, phModule ) 
#endif
{
    phModule.doc() = "Python Host Module for Megastructure";

    pybind11::class_< megastructure::ProgramWrapper, std::shared_ptr< megastructure::ProgramWrapper > >( phModule, "Program" )
        
       .def( "getHostName",         &megastructure::ProgramWrapper::getHostName )
       .def( "getProjectName",      &megastructure::ProgramWrapper::getProjectName )
       .def( "getComponentName",    &megastructure::ProgramWrapper::getComponentName )
       .def( "getComponentPath",    &megastructure::ProgramWrapper::getComponentPath )
       
       .def( "getRoot", 
               []( const megastructure::ProgramWrapper& program )
               {
                   if( void* pRoot = program.getRoot() )
                   {
                       PyObject* pPyObject = reinterpret_cast< PyObject* >( pRoot );
                       return pybind11::reinterpret_borrow< pybind11::object >( pPyObject );
                   }
                   else
                   {
                       Py_INCREF( Py_None );
                       return pybind11::reinterpret_borrow< pybind11::object >( Py_None );
                   }
               }
            )
        ;
        
        
    pybind11::class_< megastructure::Host, std::shared_ptr< megastructure::Host > >( phModule, "Host" )
        
       //.def( pybind11::init<
       //     const std::string& , 
       //     const std::string& ,
       //     const std::string& ,
       //     const std::string& 
       //     >(), "Construct a mega structure host with the provided: Project Directory, Coordinator Mega Port, Coordinator EG Port and Host Program Name" )
       
       .def( "getHostProgramName",      &megastructure::Host::getHostProgramName )
       .def( "getSlaveName",            &megastructure::Host::getSlaveName )
       .def( "getSlaveWorkspacePath",   &megastructure::Host::getSlaveWorkspacePath )
       
       .def( "getProgram",              &megastructure::Host::getProgram )
       .def( "runCycle",                &megastructure::Host::runCycle )
       ;
       
       
   phModule.def( "NewHost", &megastructure::NewHost, "Construct a new host" );
   
   phModule.def( "GetHost", &megastructure::GetHost, "Get the host" );
   
}
