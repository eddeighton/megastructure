
#include "unreal/unreal.hpp"

#include <cstdio>
#include <memory>
#include <sstream>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>

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
   
   //unreal math bindings
   
    pybind11::class_< FVector2D >( phModule, "FVector2D" )
        .def( pybind11::init<float,float>() )
        .def( pybind11::init<FVector2D>() )
        .def_readwrite("X", &FVector2D::X)
        .def_readwrite("Y", &FVector2D::Y)
        
        .def("__repr__",
            []( const FVector2D& v ) {
                std::ostringstream os;
                os << "< FVector2D: " << v.X << ',' << v.Y << " >";
                return os.str();
            })
            
        .def( "Equals", &FVector2D::Equals )
        .def( "Set", &FVector2D::Set )
        
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        .def( pybind11::self - pybind11::self )
        .def( pybind11::self -= pybind11::self )
        .def( pybind11::self * float() )
        .def( pybind11::self / float() )
        .def( pybind11::self + float() )
        .def( pybind11::self - float() )
        ;
   
    pybind11::class_< FVector >( phModule, "FVector" )
        .def( pybind11::init<float,float,float>() )
        .def( pybind11::init<FVector2D,float>() )
        .def( pybind11::init<FVector>() )
        .def( pybind11::init<FVector4>() )
        .def_readwrite("X", &FVector::X)
        .def_readwrite("Y", &FVector::Y)
        .def_readwrite("Z", &FVector::Z)
        
        .def("__repr__",
            []( const FVector& v ) {
                std::ostringstream os;
                os << "< FVector: " << v.X << ',' << v.Y << ',' << v.Z << " >";
                return os.str();
            })
            
        .def( "Equals", &FVector::Equals )
        .def( "Set", &FVector::Set )
        
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        .def( pybind11::self - pybind11::self )
        .def( pybind11::self -= pybind11::self )
        .def( pybind11::self * float() )
        .def( pybind11::self / float() )
        .def( pybind11::self + float() )
        .def( pybind11::self - float() )
        ;
        
    pybind11::class_< FVector4 >( phModule, "FVector4" )
        .def( pybind11::init<float,float,float,float>() )
        .def( pybind11::init<FVector2D,FVector2D>() )
        .def( pybind11::init<FVector,float>() )
        .def( pybind11::init<FVector4>() )
        .def_readwrite("X", &FVector4::X)
        .def_readwrite("Y", &FVector4::Y)
        .def_readwrite("Z", &FVector4::Z)
        .def_readwrite("W", &FVector4::W)
        
        .def("__repr__",
            []( const FVector4& v ) {
                std::ostringstream os;
                os << "< FVector4: " << v.X << ',' << v.Y << ',' << v.Z << ',' << v.W << " >";
                return os.str();
            })
            
        .def( "Equals", &FVector4::Equals )
        .def( "Set", &FVector4::Set )
        
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        .def( pybind11::self - pybind11::self )
        .def( pybind11::self -= pybind11::self )
        /*
        .def( pybind11::self * float() )
        .def( pybind11::self / float() )
        .def( pybind11::self + float() )
        .def( pybind11::self - float() )
        */
        ;
   
    pybind11::class_< FQuat >( phModule, "FQuat" )
        .def( pybind11::init<float,float,float,float>() )
        .def( pybind11::init<FVector,float>() )
        .def( pybind11::init<FQuat>() )
        .def_readwrite("X", &FQuat::X)
        .def_readwrite("Y", &FQuat::Y)
        .def_readwrite("Z", &FQuat::Z)
        .def_readwrite("W", &FQuat::W)
        
        .def("__repr__",
            []( const FQuat& v ) {
                std::ostringstream os;
                os << "< FQuat: " << v.X << ',' << v.Y << ',' << v.Z << ',' << v.W << " >";
                return os.str();
            })
            
        .def( "Equals", &FQuat::Equals )
        
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        .def( pybind11::self - pybind11::self )
        .def( pybind11::self -= pybind11::self )
        /*.def( pybind11::self * float() )
        .def( pybind11::self / float() )
        .def( pybind11::self + float() )
        .def( pybind11::self - float() )*/
        ;
        
   
    pybind11::class_< FMatrix >( phModule, "FMatrix" )
        .def( pybind11::init<FMatrix>() )
        .def( pybind11::init<FVector,FVector,FVector,FVector>() )
        
        
        .def("__repr__",
            []( const FMatrix& v ) {
                std::ostringstream os;
                os << "< FMatrix: ";
                for( int r = 0; r != 4; ++r )
                {
                    os << "{";
                    for( int c = 0; c != 4; ++c )
                    {
                        if( c > 0 ) 
                            os << ",";
                        os << v.M[r][c];
                    }
                    os << "}";
                }
                os << " >";
                return os.str();
            })
            
        .def( "Equals", &FMatrix::Equals )
        .def( pybind11::self * pybind11::self )
        .def( pybind11::self *= pybind11::self )
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        
        .def( "TransformFVector4", &FMatrix::TransformFVector4 )
        .def( "TransformPosition", &FMatrix::TransformPosition )
        .def( "InverseTransformPosition", &FMatrix::InverseTransformPosition )
        .def( "TransformVector", &FMatrix::TransformVector )
        .def( "InverseTransformVector", &FMatrix::InverseTransformVector )
        .def( "Inverse", &FMatrix::Inverse )
        ;
        
    pybind11::class_< FTransform >( phModule, "FTransform" )
        .def( pybind11::init<FTransform>() )
        .def( pybind11::init<FVector>() )
        .def( pybind11::init<FQuat>() )
        .def( pybind11::init<FQuat,FVector,FVector>() )
        
        .def("__repr__",
            []( const FTransform& v ) {
                std::ostringstream os;
                os << "< FTransform: ";
                const FQuat   fq = v.GetRotation();
                const FVector ft = v.GetTranslation();
                const FVector fs = v.GetScale3D();
                os << " rotat{ " << fq.X << ',' << fq.Y << ',' << fq.Z << ',' << fq.W << " }";
                os << " trans{ " << ft.X << ',' << ft.Y << ',' << ft.Z << " }";
                os << " scale{ " << fs.X << ',' << fs.Y << ',' << fs.Z << " }";
                os << " >";
                return os.str();
            })
            
        .def( "Equals", &FTransform::Equals )
        .def( pybind11::self + pybind11::self )
        .def( pybind11::self += pybind11::self )
        .def( pybind11::self * pybind11::self )
        .def( pybind11::self *= pybind11::self )
        .def( pybind11::self * FQuat() )
        .def( pybind11::self *= FQuat() )
        
        .def( "ToMatrixWithScale", &FTransform::ToMatrixWithScale )
        .def( "ToInverseMatrixWithScale", &FTransform::ToInverseMatrixWithScale )
        .def( "GetLocation", &FTransform::GetLocation )
        .def( "SetLocation", &FTransform::SetLocation )
        .def( "SetComponents", &FTransform::SetComponents )
        .def( "SetIdentity", &FTransform::SetIdentity )
        .def( "GetRotation", &FTransform::GetRotation )
        .def( "GetTranslation", &FTransform::GetTranslation )
        .def( "GetScale3D", &FTransform::GetScale3D )
        .def( "SetRotation", &FTransform::SetRotation )
        .def( "SetTranslation", &FTransform::SetTranslation )
        .def( "SetScale3D", &FTransform::SetScale3D )
        ;
}
