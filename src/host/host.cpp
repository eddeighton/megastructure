
#include "host/host.hpp"

#include <boost/filesystem.hpp>
#include <boost/optional.hpp>

#include "common/processID.hpp"
#include "common/assert_verify.hpp"

#include "protocol/protocol_helpers.hpp"

#include "megastructure/component.hpp"
#include "megastructure/mega.hpp"
#include "egcomponent/egcomponent.hpp"

namespace
{
    class HostImpl : public megastructure::IMegaHost
    {
        Environment m_environment;
        megastructure::Component m_component;
    public:
        HostImpl(   const std::string& strMegaPort, 
                    const std::string& strEGPort, 
                    const std::string& strProgramName )
            :   m_component( m_environment, strMegaPort, strEGPort, strProgramName )
        {
        }
        
        virtual ~HostImpl()
        {
            
        }
        
        virtual void runCycle()
        {
            m_component.runCycle();
        }
        
    };
}

namespace megastructure
{

IMegaHost::~IMegaHost()
{

}

IMegaHost* createMegaHost( void* pEngineInterface )
{
    return new HostImpl( "1001", "1002", "test_host_dll.exe" );
}

void destroyMegaHost( const IMegaHost* pHost )
{
    delete pHost;
}

}