
#include "megastructure/buffer.hpp"

#include "boost/interprocess/mapped_region.hpp"
#include "boost/interprocess/windows_shared_memory.hpp"

namespace megastructure
{
    
LocalBufferImpl::LocalBufferImpl( const std::string& strName, std::size_t szSize )
    :   m_strName( strName ),
        m_data( szSize )
{
}
void LocalBufferImpl::Release()
{
            
}
const char* LocalBufferImpl::getName()
{
    return m_strName.c_str();
}
void* LocalBufferImpl::getData()
{
    return reinterpret_cast< void* >( m_data.data() );
}
std::size_t LocalBufferImpl::getSize()
{
    return m_data.size();
}


class SharedBufferImpl::SharedBufferPimpl
{
    std::string m_strName;
    std::size_t m_szSize;
    boost::interprocess::windows_shared_memory m_memory;
    boost::interprocess::mapped_region m_memoryMap;

public:
    SharedBufferPimpl( const std::string& strName, std::size_t szSize )
        :   m_strName( strName ),
            m_szSize( szSize ),
            m_memory( boost::interprocess::open_or_create, 
                    m_strName.c_str(), 
                    boost::interprocess::read_write,
                    m_szSize ),
          m_memoryMap( m_memory, boost::interprocess::read_write )
    {
        memset( get(), 0, m_szSize );
    }
    
    const std::string& name() const { return m_strName; }
    std::size_t size() const { return m_szSize; }
    void* get() const { return m_memoryMap.get_address(); }
};

SharedBufferImpl::SharedBufferImpl( const std::string& strName, const std::string& strSharedName, std::size_t szSize )
    :   m_strName( strName ),
        m_pImpl( std::make_shared< SharedBufferPimpl >( strSharedName, szSize ) )
{
}

//SharedBuffer
void SharedBufferImpl::Release()
{
    //m_pImpl.reset();
}

const char* SharedBufferImpl::getName()
{
    return m_strName.c_str();
}

void* SharedBufferImpl::getData()
{
    return m_pImpl->get();
}

std::size_t SharedBufferImpl::getSize()
{
    return m_pImpl->size();
}

const std::string& SharedBufferImpl::getSharedName() const 
{ 
    return m_pImpl->name();

}

}