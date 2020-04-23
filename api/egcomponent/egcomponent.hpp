

#ifndef EG_COMPONENT_23_APRIL_2020
#define EG_COMPONENT_23_APRIL_2020
//corona virus was here!!

#include "boost/fiber/all.hpp"

#include <string>

#ifdef MEGASTRUCTURE_EG_COMPONENT
#define MEGASTRUCTURE_EG_API __declspec( dllexport )
#else
#define MEGASTRUCTURE_EG_API __declspec( dllimport )
#endif
	
namespace megastructure
{
	
	class MEGASTRUCTURE_EG_API Buffer
	{
	public:
		virtual ~Buffer() = 0;
		
		virtual void Release() = 0;
		
		virtual const char* getName() = 0;
		virtual void* getData() = 0;
		virtual std::size_t getSize() = 0;
	};
	
	class MEGASTRUCTURE_EG_API SharedBuffer : public Buffer
	{
	public:
	};
	
	class MEGASTRUCTURE_EG_API LocalBuffer : public Buffer
	{
	public:
	};
	
	class MEGASTRUCTURE_EG_API MemorySystem
	{
	public:
		virtual ~MemorySystem() = 0;
		
		virtual Buffer* getSharedBuffer( const char* pszName, std::size_t szSize ) = 0;
		virtual Buffer* getLocalBuffer( const char* pszName, std::size_t szSize ) = 0;
	};
	
	
	class MEGASTRUCTURE_EG_API MegaProtocol
	{
	public:
		virtual ~MegaProtocol() = 0;
		
		virtual boost::fibers::future< std::string > Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance ) = 0;
		
		virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		
	};
	
	
	class MEGASTRUCTURE_EG_API EGComponent
	{
	public:
		virtual ~EGComponent() = 0;		
		
		virtual void Initialise( MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol ) = 0;
		virtual void Uninitialise() = 0 ;
		
		virtual void Cycle() = 0;
		
		virtual void Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance, std::string& buffer ) = 0;
		virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
	};


}

#endif //EG_COMPONENT_23_APRIL_2020