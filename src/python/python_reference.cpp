
#include "python/python_reference.hpp"
#include "python/python_reference_factory.hpp"

#include "common/assert_verify.hpp"

#include <pybind11/pybind11.h>

#include <sstream>
#include <exception>

namespace megastructure
{


PythonEGReference::PythonEGReference( PythonEGReferenceFactory& pythonReferenceFactory, const eg::reference& ref ) 
    :   m_pythonReferenceFactory( pythonReferenceFactory ),
        m_reference( ref ) 
{
}
    
PyObject* PythonEGReference::get( void* pClosure )
{
    const char* pszAttributeIdentity = reinterpret_cast< char* >( pClosure );
    const eg::TypeID typeID = m_pythonReferenceFactory.getTypeID( pszAttributeIdentity );
    if( typeID == 0 )
    {
        std::ostringstream os;
        os << "Invalid identity" << pszAttributeIdentity;
        throw std::runtime_error( os.str() );
        
        //Py_INCREF( Py_None );
        //return Py_None;
    }
    else
    {
        PyObject* pResult = m_pythonReferenceFactory.create( m_reference );
        {
            PythonEGReference* pNewRef = PythonEGReferenceFactory::getReference( pResult );
            pNewRef->m_type_path.reserve( m_type_path.size() + 1U );
            pNewRef->m_type_path = m_type_path;
            pNewRef->m_type_path.push_back( typeID );
        }
        
        return pResult;
    }
}

int PythonEGReference::set( void* pClosure, PyObject* pValue )
{
    //const char* pszAttributeIdentity = reinterpret_cast< char* >( pClosure );
    return 0;
}

PyObject* PythonEGReference::str() const
{
    std::ostringstream os;
    os << "instance: " << m_reference.instance << " type: " << m_reference.type << " timestamp: " << m_reference.timestamp;
    for( std::vector< eg::TypeID >::const_iterator 
        i = m_type_path.begin(), iEnd = m_type_path.end(); i!=iEnd; ++i )
    {
        if( i == m_type_path.begin() )
        {
            os << " type path: ";
        }
        os << *i << " ";
    }
    return Py_BuildValue( "s", os.str().c_str() );
}

PyObject* PythonEGReference::call( PyObject *args, PyObject *kwargs )
{
    return m_pythonReferenceFactory.invoke( m_reference, m_type_path, args, kwargs );
}



} //namespace megastructure