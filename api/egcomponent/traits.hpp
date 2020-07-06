

#ifndef MEGA_TRAITS_05_MAY_2020
#define MEGA_TRAITS_05_MAY_2020

#include "eg/common.hpp"

#include "msgpack.hpp"

#include <vector>
#include <bitset>
#include <type_traits>

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
    static const std::size_t Simple = std::is_trivially_copyable< T >::value;
	
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
    static void readlock( eg::TypeID component );
    static void writelock( eg::TypeID component );
    static void read( eg::TypeID component, eg::TypeID type, eg::Instance instance );
};

template< typename T, std::size_t szComponentBit, std::size_t szComponentRTT >
inline T& readlock( T& value )
{
    if( !g_hostLocks.test( szComponentBit ) )
    {
        Component::readlock( szComponentRTT );
        g_hostLocks.set( szComponentBit );
    }
    return value;
}

template< typename T, std::size_t szComponentBit, eg::TypeID szComponentRTT, std::size_t szHashBase, eg::TypeID szType >
inline T& readlock( eg::Instance instance, T& value )
{
    if( !g_hostLocks.test( szComponentBit ) )
    {
        Component::read( szComponentRTT, szType, instance );
        g_hostLocks.set( szComponentBit );
        g_reads.set( szHashBase + instance );
    }
    else if( !g_reads.test( szHashBase + instance ) )
    {
        Component::read( szComponentRTT, szType, instance );
        g_reads.set( szHashBase + instance );
    }
    return value;
}

template< typename T, std::size_t szComponentBit, eg::TypeID szComponentRTT >
inline T& writelock( T& value )
{
    if( !g_hostLocks.test( szComponentBit ) )
    {
        Component::writelock( szComponentRTT );
        g_hostLocks.set( szComponentBit );
    }
    return value;
}

template< typename T, std::size_t szComponentBit, eg::TypeID szComponentRTT, eg::TypeID szTypeID >
inline T& writelock( eg::Instance instance, std::set< eg::TypeInstance >& writeSet, T& value )
{
    if( !g_hostLocks.test( szComponentBit ) )
    {
        Component::writelock( szComponentRTT );
        g_hostLocks.set( szComponentBit );
    }
    writeSet.insert( TypeInstance{ instance, szTypeID } );
    return value;
}

}

#endif //MEGA_TRAITS_05_MAY_2020