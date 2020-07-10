
#include "eg/common.hpp"

#include <vector>

#include <pybind11/pybind11.h>

namespace megastructure
{
class PythonEGReferenceFactory;

class PythonEGReference
{
public:
    PythonEGReference( PythonEGReferenceFactory& pythonReferenceFactory, const eg::reference& ref );
    
    PyObject* get( void* pClosure );
    int set( void* pClosure, PyObject* pValue );
    PyObject* str() const;
    PyObject* call( PyObject *args, PyObject *kwargs );
    
    const eg::reference getEGReference() const { return m_reference; }
private:
    PythonEGReferenceFactory& m_pythonReferenceFactory;
    eg::reference m_reference;
    std::vector< eg::TypeID > m_type_path;
};

}
