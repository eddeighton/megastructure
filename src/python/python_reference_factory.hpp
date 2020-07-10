
#include "eg_runtime/eg_runtime.hpp"

#include <pybind11/pybind11.h>

namespace megastructure
{
    
class PythonEGReference;

class PythonEGReferenceFactory
{
public:
    static PythonEGReference* getReference( PyObject* pPyObject );

    PythonEGReferenceFactory( const char* pszModuleName, eg::EGRuntime& egRuntime, eg::RuntimeTypeInterop& runtimeInterop );
    ~PythonEGReferenceFactory();
    
    PyObject* create( eg::reference ref );
    
    eg::TypeID getTypeID( const char* pszIdentity );
    PyObject* invoke( const eg::reference& reference, const std::vector< eg::TypeID >& typePath, PyObject *args, PyObject *kwargs );
    
    eg::TimeStamp getTimestamp( eg::TypeID type, eg::Instance instance );
    eg::ActionState getState( eg::TypeID type, eg::Instance instance );
    eg::TimeStamp getStopCycle( eg::TypeID type, eg::Instance instance );
    eg::TimeStamp cycle();
        
private:
    eg::EGRuntime& m_egRuntime;
    eg::RuntimeTypeInterop& m_runtimeInterop;
    PyTypeObject* m_pTypeObject;
    std::vector< PyGetSetDef > m_pythonAttributesData;
    std::vector< const char* > m_identities;
};



}