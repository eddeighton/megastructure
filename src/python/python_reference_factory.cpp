
#include "python/python_reference_factory.hpp"
#include "python/python_reference.hpp"

#include "common/assert_verify.hpp"
#include "eg/macros.hpp"

#include <pybind11/pybind11.h>

namespace megastructure
{
    
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
typedef struct 
{
    PyObject_HEAD
    PythonEGReference* pReference;
}eg_reference_data;

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

PythonEGReference* PythonEGReferenceFactory::getReference( PyObject* pPyObject )
{
    eg_reference_data* pLogicalObject = (eg_reference_data*)pPyObject;
    return pLogicalObject->pReference;
}

static void type_dealloc( PyObject* pPyObject )
{
    PythonEGReference* pReference = PythonEGReferenceFactory::getReference( pPyObject );
    delete pReference;
    Py_TYPE( pPyObject )->tp_free( (PyObject*)pPyObject );
}

static PyObject* type_get( PyObject* self, void* pClosure )
{
    if( PythonEGReference* pRef = PythonEGReferenceFactory::getReference( self ) )
    {
        return pRef->get( pClosure );
    }
    else
    {
        //PYTHON_ERROR( "PythonEGReferenceFactory is out of date" );
        return 0;
    }
}

static int type_set( PyObject* self, PyObject* pValue, void* pClosure )
{
    if( PythonEGReference* pRef = PythonEGReferenceFactory::getReference( self ) )
    {
        return pRef->set( pClosure, pValue );
    }
    else
    {
        //PYTHON_ERROR( "PythonEGReferenceFactory is out of date" );
        return -1;
    }
}

PyObject* type_str( PyObject* self )
{
    if( PythonEGReference* pRef = PythonEGReferenceFactory::getReference( self ) )
    {
        return pRef->str();
    }
    else
    {
        //PYTHON_ERROR( "PythonEGReferenceFactory is out of date" );
        return NULL;
    }
}

PyObject* type_call( PyObject *callable, PyObject *args, PyObject *kwargs )
{
    if( PythonEGReference* pRef = PythonEGReferenceFactory::getReference( callable ) )
    {
        return pRef->call( args, kwargs );
    }
    else
    {
        //PYTHON_ERROR( "PythonEGReferenceFactory is out of date" );
        return NULL;
    }
}
    
static PyMethodDef type_methods[] = 
{
    //{"sample", (PyCFunction)type_sample, METH_VARARGS, "Sample the object and its subtree" },
    //{"assign", (PyCFunction)type_assign, METH_VARARGS, "Assign the object to a sample" },
    //{"update", (PyCFunction)type_update, METH_NOARGS, "Update objects associated gpu buffers" },
    {NULL}  /* Sentinel */
};

PythonEGReferenceFactory::PythonEGReferenceFactory( const char* pszModuleName,
            eg::EGRuntime& egRuntime, eg::RuntimeTypeInterop& runtimeInterop )
    :   m_egRuntime( egRuntime ),
        m_runtimeInterop( runtimeInterop ),
        m_pTypeObject( nullptr )
{        
    m_egRuntime.getIdentities( m_identities );
    for( const char* pszIdentity : m_identities )
    {
        char* pszNonConst = const_cast< char* >( pszIdentity );
        PyGetSetDef data =
        { 
            pszNonConst, 
            (getter)type_get, 
            (setter)type_set, 
            pszNonConst, 
            (void*)pszNonConst
        };
        m_pythonAttributesData.push_back( data );
    }
    m_pythonAttributesData.push_back( PyGetSetDef{ NULL } );
    
    //generate heap allocated python type...
    std::vector< PyType_Slot > slots;
    {
        slots.push_back( PyType_Slot{ Py_tp_str,        reinterpret_cast< void*>( &type_str ) } );
        slots.push_back( PyType_Slot{ Py_tp_repr,       reinterpret_cast< void*>( &type_str ) } );
        slots.push_back( PyType_Slot{ Py_tp_dealloc,    reinterpret_cast< void*>( &type_dealloc ) } );
        slots.push_back( PyType_Slot{ Py_tp_call,       reinterpret_cast< void*>( &type_call ) } );
        slots.push_back( PyType_Slot{ Py_tp_methods,    reinterpret_cast< void*>( &type_methods ) } );
        slots.push_back( PyType_Slot{ Py_tp_getset,     m_pythonAttributesData.data() } );
    }
    
    slots.push_back( PyType_Slot{ 0 } );
    
    static std::string strTypeName;
    {
        std::ostringstream osTypeName;
        osTypeName << pszModuleName << ".reference";
        strTypeName = osTypeName.str();
    }
    
    PyType_Spec spec = 
    { 
        strTypeName.c_str(),
        sizeof( eg_reference_data ), 
        0, 
        Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, 
        slots.data() 
    };
    
    m_pTypeObject = (PyTypeObject*)PyType_FromSpec( &spec );        
    
    if( PyType_Ready( m_pTypeObject ) < 0 )
    {
        //set exception
        THROW_RTE( "Failed to create python egreference type" );
    }
    else
    {
        Py_INCREF( m_pTypeObject );
        //successfully generated the dynamic type...
        //PyModule_AddObject( pPythonModule, "Host", (PyObject*)&m_type );
    }
}

PythonEGReferenceFactory::~PythonEGReferenceFactory()
{
    if( m_pTypeObject )
    {
        Py_DECREF( m_pTypeObject );
    }
}
    
PyObject* PythonEGReferenceFactory::create( eg::reference egReference )
{
    eg_reference_data* pRootObject = PyObject_New( eg_reference_data, m_pTypeObject );
    PyObject* pPythonObject = PyObject_Init( (PyObject*)pRootObject, m_pTypeObject );
    pRootObject->pReference = new PythonEGReference( *this, egReference );
    Py_INCREF( pPythonObject );
    return pPythonObject;
}

eg::TypeID PythonEGReferenceFactory::getTypeID( const char* pszIdentity )
{
    return m_egRuntime.getTypeID( pszIdentity );
}

PyObject* PythonEGReferenceFactory::invoke( const eg::reference& reference, const std::vector< eg::TypeID >& typePath, PyObject *args, PyObject *kwargs )
{
    try
    {
        if( reference.type != 0 )
        {
            eg::RuntimeTypeInterop::Evaluation::Ptr pEvaluation = 
                m_runtimeInterop.begin( args, kwargs );
            
            pybind11::args pyArgs = pybind11::reinterpret_borrow< pybind11::args >( args );
            
            //actually invoke the emulator to evaluate the invocation
            m_egRuntime.invoke( reference, typePath, pyArgs.size() != 0 );
            
            if( pEvaluation->isResult() )
            {
                return reinterpret_cast< PyObject* >( pEvaluation->getResult() );
            }
        }
        else
        {
            ERR( "Invalid reference used in invocation" );
        }
    }
    catch( std::exception& ex )
    {
        ERR( "Invocation Error: " << ex.what() );
    }
    Py_INCREF( Py_None );
    return Py_None;
    
}

eg::TimeStamp PythonEGReferenceFactory::getTimestamp( eg::TypeID type, eg::Instance instance )
{
    return m_runtimeInterop.getTimestamp( type, instance );
}

eg::ActionState PythonEGReferenceFactory::getState( eg::TypeID type, eg::Instance instance )
{
    return m_runtimeInterop.getState( type, instance );
}

eg::TimeStamp PythonEGReferenceFactory::getStopCycle( eg::TypeID type, eg::Instance instance )
{
    return m_runtimeInterop.getStopCycle( type, instance );
}
    
eg::TimeStamp PythonEGReferenceFactory::cycle( eg::TypeID type )
{
    return m_runtimeInterop.getClockCycle( type );
}
    
}