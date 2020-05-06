

#ifndef EG_COMPONENT_23_APRIL_2020
#define EG_COMPONENT_23_APRIL_2020
//corona virus was here!!

#include "traits.hpp"

//#include "boost/fiber/all.hpp"
#include "boost/config.hpp"

#include <string>
#include <vector>

namespace megastructure
{
	
	class BOOST_SYMBOL_VISIBLE EncodeDecode
	{
	public:
		virtual void encode( std::uint32_t uiType, std::uint32_t uiInstance, eg::Encoder& encoder ) = 0;
		virtual void decode( std::uint32_t uiType, std::uint32_t uiInstance, eg::Decoder& decoder ) = 0;
	};
	
	class BOOST_SYMBOL_VISIBLE Buffer
	{
	public:
		virtual void Release() = 0;
		
		virtual const char* getName() = 0;
		virtual void* getData() = 0;
		virtual std::size_t getSize() = 0;
	};
	
	class BOOST_SYMBOL_VISIBLE SharedBuffer : public Buffer
	{
	public:
	};
	
	class BOOST_SYMBOL_VISIBLE LocalBuffer : public Buffer
	{
	public:
	};
	
	class BOOST_SYMBOL_VISIBLE MemorySystem
	{
	public:
		virtual SharedBuffer* getSharedBuffer( const char* pszName, std::size_t szSize ) = 0;
		virtual LocalBuffer* getLocalBuffer( const char* pszName, std::size_t szSize ) = 0;
	};
	
	class BOOST_SYMBOL_VISIBLE MegaProtocol
	{
	public:
		//virtual boost::fibers::future< std::string > Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance ) = 0;
		
		virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		
	};
	
	class BOOST_SYMBOL_VISIBLE EGComponent
	{
	public:
		virtual void Initialise( EncodeDecode*& pEncodeDecode, MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol ) = 0;
		virtual void Uninitialise() = 0 ;
		
		virtual void Cycle() = 0;
		/*
		virtual void Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance, std::string& buffer ) = 0;
		virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer ) = 0;
		virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance ) = 0;
		*/
	};


}

#endif //EG_COMPONENT_23_APRIL_2020