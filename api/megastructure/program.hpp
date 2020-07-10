

#ifndef EG_PROGRAM_01_MAY_2020
#define EG_PROGRAM_01_MAY_2020

#include "megastructure/component.hpp"
#include "megastructure/buffer.hpp"
#include "megastructure/networkAddressTable.hpp"

#include "protocol/megastructure.pb.h"
#include "protocol/protocol_helpers.hpp"

#include "egcomponent/egcomponent.hpp"

#include "boost/shared_ptr.hpp"

#include <memory>
#include <string>
#include <map>

class ProjectTree;

namespace megastructure
{
	class Program : public MemorySystem, public MegaProtocol, public std::enable_shared_from_this< Program >
	{
        using CacheBufferMap = std::map< std::string, LocalBufferImpl::Ptr >;
        using SharedBufferMap = std::map< std::string, SharedBufferImpl::Ptr >;

		friend class EGReadFunctor;
	public:
		using Ptr = std::shared_ptr< Program >;
	
		Program( Component& component, const std::string& strHostName, const std::string& strProjectName );
		~Program();
        
        const std::string& getHostName() const { return m_strHostName; }
        const std::string& getProjectName() const { return m_strProjectName; }
        const std::string& getComponentName() const { return m_strComponentName; }
        const boost::filesystem::path& getComponentPath() const { return m_componentPath; }
        
        void* getRoot() const;
		
		void run();
        
        //MemorySystem
		virtual SharedBuffer* getSharedBuffer( const char* pszName, std::size_t szSize );
		virtual LocalBuffer* getLocalBuffer( const char* pszName, std::size_t szSize );
        
        //MegaProtocol
		//virtual bool receive( std::int32_t& iType, std::uint32_t& uiInstance, std::uint32_t& uiTimestamp );
		//virtual void send( const char* type, std::size_t timestamp, const void* value, std::size_t size );
		
		//test routines
        /*
		std::string egRead( std::int32_t iType, std::uint32_t uiInstance );
		void egWrite( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer );
		void egCall( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer );
		*/
        
		//memory access
		void readBuffer( std::int32_t iType, std::uint32_t uiInstance, std::string& strBuffer );
		void writeBuffer( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer );
		
	private:
		Component& m_component;
		std::string m_strHostName;
		std::string m_strProjectName;
		
		std::shared_ptr< ProjectTree > m_pProjectTree;
		NetworkAddressTable::Ptr m_pNetworkAddressTable;
		ProgramTypeTable::Ptr m_pProgramTable;
		
		std::string m_strComponentName;
		boost::filesystem::path m_componentPath;
		
		boost::shared_ptr< megastructure::EGComponent > m_pPlugin;
		EncodeDecode* m_pEncodeDecode;
        
        CacheBufferMap m_cacheBuffers;
        SharedBufferMap m_sharedBuffers;
	};
}

#endif //EG_PROGRAM_01_MAY_2020