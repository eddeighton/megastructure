

#ifndef MEGA_TRAITS_05_MAY_2020
#define MEGA_TRAITS_05_MAY_2020

#include "eg/common.hpp"

#include "msgpack.hpp"

#include <vector>
#include <bitset>

namespace eg
{
	
using Encoder = msgpack::packer< msgpack::sbuffer >;
using Decoder = msgpack::unpacker;
	
template< typename T >
inline void encode( Encoder& encoder, const T& value )
{
	encoder.pack( value );
}

	
template< typename T >
inline void decode( Decoder& decoder, T& value )
{
	msgpack::object_handle objectHandle;
	if( decoder.next( objectHandle ) )
	{
		objectHandle.get().convert( value );
	}
	else
	{
		throw std::runtime_error( "Decoder error" );
	}
}


template<>
inline void encode( Encoder& encoder, const eg::ActionState& value )
{
	std::uint8_t ui = static_cast< std::uint8_t >( value );
	encode( encoder, ui );
}

template<>
inline void decode( Decoder& decoder, eg::ActionState& value )
{
	std::uint8_t ui;
	decode( decoder, ui );
	value = static_cast< eg::ActionState >( ui );
}


template<>
inline void encode( Encoder& encoder, const eg::reference& value )
{
	encode( encoder, value.instance );
	encode( encoder, value.type );
	encode( encoder, value.timestamp );
}

template<>
inline void decode( Decoder& decoder, eg::reference& value )
{
	decode( decoder, value.instance );
	decode( decoder, value.type );
	decode( decoder, value.timestamp );
}


template< typename T >
struct DimensionTraits
{
    using Read  = const T&;
    using Write = T;
    using Get   = T&;
    static const std::size_t Size = sizeof( T );
	
    static inline void initialise( T& value )
    {
        new (&value ) T;
    }
    static void uninitialise( T& value )
    {
        value.~T();
    }
	
	static inline void encode( Encoder& encoder, const T& value )
	{
		::eg::encode< T >( encoder, value );
	}
	
	static inline void decode( Decoder& decoder, T& value )
	{
		::eg::decode< T >( decoder, value );
	}
};

class Component
{
public:
    static void read( eg::TypeID type, eg::Instance instance );
};

template< typename T, std::size_t szHashBase >
inline T& read( eg::TypeID type, eg::Instance instance, T& value )
{
    if( !g_reads.test( szHashBase + instance ) )
    {
        //Component::read( type, instance );
        g_reads.set( szHashBase + instance );
    }
    return value;
}

}

#endif //MEGA_TRAITS_05_MAY_2020