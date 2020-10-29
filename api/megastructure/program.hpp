

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
#include <utility>

class ProjectTree;

namespace megastructure
{
	class Program : public MemorySystem, public MegaProtocol, public EventLog, public std::enable_shared_from_this< Program >
	{
        using CacheBufferMap = std::map< std::string, LocalBufferImpl::Ptr >;
        using SharedBufferMap = std::map< std::string, SharedBufferImpl::Ptr >;
        using ComponentLockInfo = std::pair< std::int32_t, std::uint32_t >;
        using ComponentLockSet = std::set< ComponentLockInfo >;

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
        eg::TimeStamp getCurrentCycle() const;
		
		void run();
        
        //MemorySystem
		virtual SharedBuffer* getSharedBuffer( const char* pszName, std::size_t szSize );
		virtual LocalBuffer* getLocalBuffer( const char* pszName, std::size_t szSize );
        
        //MegaProtocol
        virtual eg::TimeStamp readlock( eg::TypeID component, std::uint32_t uiTimestamp );
        virtual void read( eg::TypeID type, std::uint32_t& uiInstance, std::uint32_t uiTimestamp );
		virtual eg::TimeStamp writelock( eg::TypeID component, std::uint32_t uiTimestamp );
		virtual void write( eg::TypeID component, const char* pBuffer, std::size_t szSize, std::uint32_t uiTimestamp );
		
		//request handling
		void readRequest( std::int32_t iType, std::uint32_t uiInstance, std::string& strBuffer );
        void readResponse( std::int32_t iType, std::uint32_t uiInstance, const std::string& strBuffer );
		void writeRequest( const std::string& strBuffer );
        
        //event log
        virtual void put( const char* type, eg::TimeStamp timestamp, const void* value, std::size_t size );
        
        //config
        void loadConfig();
        void saveConfig();
    private:
        void releaseLocks();
		
	private:
		Component& m_component;
		std::string m_strHostName;
		std::string m_strProjectName;
		
		std::shared_ptr< ProjectTree > m_pProjectTree;
		NetworkAddressTable::Ptr m_pNetworkAddressTable;
		ProgramTypeTable::Ptr m_pProgramTable;
		
		std::string m_strComponentName;
		boost::filesystem::path m_componentPath;
		boost::filesystem::path m_componentSrcPath;
		
		boost::shared_ptr< megastructure::EGComponent > m_pPlugin;
		EncodeDecode* m_pEncodeDecode;
        ConfigIO* m_pConfigIO;
        
        CacheBufferMap m_cacheBuffers;
        SharedBufferMap m_sharedBuffers;
        
        ComponentLockSet m_componentLocks;
	};
}

#endif //EG_PROGRAM_01_MAY_2020