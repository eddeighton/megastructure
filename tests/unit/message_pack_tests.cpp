

//#include "protocol/megastructure.pb.h"

#include <limits>

#include <gtest/gtest.h>

//#include "msgpack.hpp"

TEST( MessagePack, Basic )
{
	
	//ASSERT_TRUE( std::numeric_limits< float >::has_quiet_NaN );
	//ASSERT_TRUE( std::numeric_limits< float >::has_signaling_NaN );
	//ASSERT_TRUE( std::numeric_limits< float >::has_infinity );
	//
	//std::vector< float > values =
	//{
	//	std::numeric_limits< float >::infinity(),            // 0
	//	std::numeric_limits< float >::min(),                 // 1
	//	std::numeric_limits< float >::max(),                 // 2
	//	std::numeric_limits< float >::lowest(),              // 3
	//	std::numeric_limits< float >::denorm_min(),          // 4
	//	std::numeric_limits< float >::quiet_NaN(),        	 // 5
	//	std::numeric_limits< float >::signaling_NaN()        // 6
	//};
	//
	//
	//msgpack::sbuffer buffer;
	//
	//{
	//	msgpack::packer< msgpack::sbuffer > packer( &buffer );
	//	packer.pack( values[0] );
	//	packer.pack( values[1] );
	//	packer.pack( values[2] );
	//	packer.pack( values[3] );
	//	packer.pack( values[4] );
	//	packer.pack( values[5] );
	//	packer.pack( values[6] );
	//}
	//
	//{
	//	msgpack::unpacker unpacker;
	//	unpacker.reserve_buffer( buffer.size() );
	//	memcpy( unpacker.buffer(), buffer.data(), buffer.size() );
	//	unpacker.buffer_consumed( buffer.size() );
	//
	//	msgpack::object_handle objHandle;
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_EQ( values[0], objHandle.get().as< float >() );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_EQ( values[1], objHandle.get().as< float >() );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_EQ( values[2], objHandle.get().as< float >() );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_EQ( values[3], objHandle.get().as< float >() );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_EQ( values[4], objHandle.get().as< float >() );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_TRUE( std::isnan( objHandle.get().as< float >() ) );
	//	
	//	ASSERT_TRUE( unpacker.next( objHandle ) );
	//	ASSERT_TRUE( std::isnan( objHandle.get().as< float >() ) );
	//
	//	ASSERT_TRUE( !unpacker.next( objHandle ) );
	//}
	
}
