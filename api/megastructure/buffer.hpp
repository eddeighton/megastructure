
#ifndef BUFFER_02_MAY_2020
#define BUFFER_02_MAY_2020

#include "egcomponent/egcomponent.hpp"

#include <memory>
#include <vector>
#include <string>

namespace megastructure
{
    class LocalBufferImpl : public LocalBuffer
    {
        using Data = std::vector< std::uint8_t >;
    public:
        using Ptr = std::shared_ptr< LocalBufferImpl >;
        
        LocalBufferImpl( const std::string& strName, std::size_t szSize );
        virtual void Release();
        virtual const char* getName();
        virtual void* getData();
        virtual std::size_t getSize();
    private:
        std::string m_strName;
        Data m_data;
    };

    class SharedBufferImpl : public SharedBuffer
    {
        class SharedBufferPimpl;
    public:
        using Ptr = std::shared_ptr< SharedBufferImpl >;
        
        SharedBufferImpl( const std::string& strName, const std::string& strSharedName, std::size_t szSize );
        
        virtual void Release();
        virtual const char* getName();
        virtual void* getData();
        virtual std::size_t getSize();
        const std::string& getSharedName() const;
        
    private:
        std::string m_strName;
        std::shared_ptr< SharedBufferPimpl > m_pImpl;
    };
    
}

#endif //BUFFER_02_MAY_2020