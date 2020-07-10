
#include "python/python_iterator.hpp"
#include "python/python_reference_factory.hpp"

#include "common/assert_verify.hpp"

#include <pybind11/pybind11.h>

namespace megastructure
{

PythonIterator::PythonIterator( const PythonIterator& from ) : 
    m_pythonType( from.m_pythonType ), 
    m_pRange( from.m_pRange ),
    m_position( from.m_position ),
    m_subRange( from.m_subRange )
{
}

PythonIterator::PythonIterator( PythonEGReferenceFactory& pythonType, eg::EGRangeDescriptionPtr pRange, bool bEnd ) 
    :   m_pythonType( pythonType ), 
        m_pRange( pRange ),
        m_position( 0U ),
        m_subRange( 0U )
{
    if( bEnd )
    {
        m_subRange = m_pRange->getSize();
    }
    else if( m_subRange != m_pRange->getSize() )
    {
        m_position = m_pRange->getBegin( m_subRange );
        scanToNextOrEnd();
    }
}

void PythonIterator::scanToNextOrEnd()
{
    while( true )
    {
        //if at end of current range and remaining subranges increment subrange
        while( ( m_subRange != m_pRange->getSize() ) && ( m_position == m_pRange->getEnd( m_subRange ) ) )
        {
            ++m_subRange;
            if( m_subRange == m_pRange->getSize() )
                break;
            m_position = m_pRange->getBegin( m_subRange );
        }
        
        //at end of all sub ranges then exit
        if( m_subRange == m_pRange->getSize() )
            break;
        
        //now actually see if the current position is valid
        if( !m_pRange->raw() )
        {
            if( ( m_pythonType.getState( m_pRange->getType( m_subRange ), m_position ) != eg::action_stopped ) || 
                ( m_pythonType.getStopCycle( m_pRange->getType( m_subRange ), m_position ) == m_pythonType.cycle() ) )
                break;
        }
        else
        {
            break;
        }
        
        //skip to next and continue scanning
        ++m_position;
    }
}

PythonIterator& PythonIterator::operator++()
{
    //ASSERT( m_subRange != m_pRange->getSize() );
    ++m_position;
    scanToNextOrEnd();
    return *this;
}

PythonIterator PythonIterator::operator++(int) 
{
    PythonIterator tmp( *this ); 
    operator++(); 
    return tmp;
}

bool PythonIterator::operator==(const PythonIterator& rhs) const 
{
    if( ( m_pRange == rhs.m_pRange ) && ( m_subRange == rhs.m_subRange ) )
    {
        if( m_subRange == m_pRange->getSize() )
            return true;
        else
            return m_position == rhs.m_position;
    }
    return false;
}

pybind11::object PythonIterator::operator*()
{
    if( m_subRange == m_pRange->getSize() )
    {
        Py_INCREF( Py_None );
        return pybind11::reinterpret_borrow< pybind11::object >( Py_None );
    }
    else
    {
        const eg::TypeID typeID = m_pRange->getType( m_subRange );
        ::eg::reference ref{ m_position, typeID, m_pythonType.getTimestamp( typeID, m_position ) };
        return pybind11::reinterpret_borrow< pybind11::object >( m_pythonType.create( ref ) );
    }
}


}