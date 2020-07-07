
#include <cstdio>
#include <memory>

#include <pybind11/pybind11.h>

#include "megastructure/component.hpp"

class Host
{
public:
    Host()
    {
        pybind11::print( "Hello python from Host ctor" );
    }
    
    
    bool IsConnected() const
    {
        return false;
    }
    
    void Run()
    {
        ++m_iCycle;
        pybind11::print( "Cycle: ", m_iCycle );
    }
private:
    int m_iCycle = 0;
};

PYBIND11_MODULE( python_host, phModule ) 
{
    phModule.doc() = "Python Host Module for Megastructure";

    //phModule.def( "foobar", &foobar, "Test function" );
    
    pybind11::class_< Host, std::shared_ptr< Host > >( phModule, "Host" )
        
       .def( pybind11::init<>() )
       
       .def( "IsConnected", &Host::IsConnected )
       .def( "Run", &Host::Run )
       
       ;
}
