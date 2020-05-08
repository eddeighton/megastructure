

#ifndef EG_COMPONENT_23_APRIL_2020
#define EG_COMPONENT_23_APRIL_2020
//corona virus was here!!

#include "traits.hpp"

//#include "boost/fiber/all.hpp"
#include "boost/config.hpp"

#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

namespace megastructure
{
	
	class BOOST_SYMBOL_VISIBLE EncodeDecode
	{
	public:
		virtual void encode( std::int32_t iType, std::uint32_t uiInstance, eg::Encoder& encoder ) = 0;
		virtual void decode( std::int32_t iType, std::uint32_t uiInstance, eg::Decoder& decoder ) = 0;
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
		virtual bool receive( std::int32_t& iType, std::uint32_t& uiInstance, std::uint32_t& uiTimestamp ) = 0;
		virtual void send( const char* type, std::size_t timestamp, const void* value, std::size_t size ) = 0;
		
	};
	
	class BOOST_SYMBOL_VISIBLE EGComponent
	{
	public:
		virtual void Initialise( EncodeDecode*& pEncodeDecode, MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol ) = 0;
		virtual void Uninitialise() = 0 ;
		
		virtual void WaitForReadResponse( std::int32_t iType, std::uint32_t uiInstance ) = 0;
		
		virtual void Cycle() = 0;
		
	};


}

#endif //EG_COMPONENT_23_APRIL_2020