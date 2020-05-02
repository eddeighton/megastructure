

#ifndef EG_PROGRAM_01_MAY_2020
#define EG_PROGRAM_01_MAY_2020

#include "megastructure/component.hpp"
#include "megastructure/buffer.hpp"

#include "egcomponent/egcomponent.hpp"

#include "boost/shared_ptr.hpp"

#include <memory>
#include <string>
#include <map>

namespace megastructure
{
	class Program : public MemorySystem, public MegaProtocol, public std::enable_shared_from_this< Program >
	{
        using CacheBufferMap = std::map< std::string, LocalBufferImpl::Ptr >;
        using SharedBufferMap = std::map< std::string, SharedBufferImpl::Ptr >;

	public:
		using Ptr = std::shared_ptr< Program >;
	
		Program( Component& component, const std::string& strHostName, const std::string& strProjectName );
		~Program();
		
		void run();
        
        //MemorySystem
		virtual SharedBuffer* getSharedBuffer( const char* pszName, std::size_t szSize );
		virtual LocalBuffer* getLocalBuffer( const char* pszName, std::size_t szSize );
        
        //MegaProtocol
        //virtual boost::fibers::future< std::string > Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance );
        virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer );
        virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer );
        virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance );
        virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance );
        virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance );
		
	private:
		Component& m_component;
		std::string m_strHostName;
		std::string m_strProjectName;
		
		std::string m_strComponentName;
		boost::filesystem::path m_componentPath;
		
		boost::shared_ptr< megastructure::EGComponent > m_pPlugin;
        
        CacheBufferMap m_cacheBuffers;
        SharedBufferMap m_sharedBuffers;
	};
}

#endif //EG_PROGRAM_01_MAY_2020