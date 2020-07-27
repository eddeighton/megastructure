
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
            m_component( m_environment, strMegaPort, strEGPort, strHostProgramName )
    {
        
    }
    
    const std::string& getHostProgramName()    const { return m_component.getHostProgramName(); }
    const std::string& getSlaveName()          const { return m_component.getSlaveName(); }
    const boost::filesystem::path& getSlaveWorkspacePath()  const { return m_component.getSlaveWorkspacePath(); }

    std::shared_ptr< Program > getProgram() { return m_component.getProgram(); }
    
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

    pybind11::class_< megastructure::Program, std::shared_ptr< megastructure::Program > >( phModule, "Program" )
        
       .def( "getHostName",         &megastructure::Program::getHostName )
       .def( "getProjectName",      &megastructure::Program::getProjectName )
       .def( "getComponentName",    &megastructure::Program::getComponentName )
       .def( "getComponentPath",    &megastructure::Program::getComponentPath )
       
       .def( "getRoot", 
               []( const megastructure::Program& program )
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
