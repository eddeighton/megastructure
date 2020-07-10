
#include "eg_runtime/eg_runtime.hpp"

#include <pybind11/pybind11.h>

#include <iterator>

namespace megastructure
{
class PythonEGReferenceFactory;

class PythonIterator : public std::iterator< std::forward_iterator_tag, pybind11::object >
{
public:
    PythonIterator( const PythonIterator& from );
    PythonIterator( PythonEGReferenceFactory& pythonType, eg::EGRangeDescriptionPtr pRange, bool bEnd );
    
    PythonIterator& operator++();
    PythonIterator operator++( int );
    
    bool operator == ( const PythonIterator& rhs ) const;
    inline bool operator != ( const PythonIterator& rhs ) const { return !(rhs==*this); }
    
    pybind11::object operator*();
    
private:
    void scanToNextOrEnd();
    
private:
    PythonEGReferenceFactory& m_pythonType;
    eg::EGRangeDescriptionPtr m_pRange;
    eg::Instance m_position, m_subRange;
};

}
