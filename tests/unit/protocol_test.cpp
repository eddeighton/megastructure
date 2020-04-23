

#include "protocol/megastructure.pb.h"

#include <gtest/gtest.h>

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <sstream>
#include <string>

#include <iostream>

TEST( Protocol, Basic )
{
	/*std::string str;
	{
		megastructure::Message request;
		request.set_msg( "test" );
		
		request.SerializeToString( &str );
		
		std::cout << str << std::endl;
	}
	
	{
		megastructure::Message request;
		request.ParseFromString( str );
		
		ASSERT_EQ( request.msg(), "test" );
	}*/
}


void protobuf_write( google::protobuf::io::CodedOutputStream& os, int value )
{
	os.WriteLittleEndian32( static_cast< google::protobuf::uint32 >( value ) );
}
void protobuf_write( google::protobuf::io::CodedOutputStream& os, unsigned int value )
{
	os.WriteLittleEndian32( static_cast< google::protobuf::uint32 >( value ) );
}

void protobuf_write( google::protobuf::io::CodedOutputStream& os, const std::string& str )
{
	google::protobuf::uint32 uiSize = str.size();
	os.WriteVarint32( uiSize );
	os.WriteString( str );
}

void protobuf_read( google::protobuf::io::CodedInputStream& is, int& value  )
{
	is.ReadLittleEndian32( reinterpret_cast< google::protobuf::uint32* >( &value ) );
}
void protobuf_read( google::protobuf::io::CodedInputStream& is, unsigned int& value  )
{
	is.ReadLittleEndian32( reinterpret_cast< google::protobuf::uint32* >( &value ) );
}

void protobuf_read( google::protobuf::io::CodedInputStream& is, std::string& str )
{
	google::protobuf::uint32 uiSize = 0;
	is.ReadVarint32( &uiSize );
	str.resize( uiSize );
	is.ReadString( &str, uiSize );
}




template< typename T >
T writeThenRead( const T& value )
{
	std::string str;
	{
		std::ostringstream os;
		{
			google::protobuf::io::OstreamOutputStream zcOutputStream( &os );
			google::protobuf::io::CodedOutputStream output( &zcOutputStream );
			protobuf_write( output, value );
		}
		str = os.str();
	}
	
	T readValue;
	{
		std::istringstream is( str );
		google::protobuf::io::IstreamInputStream zcInputStream( &is );
		google::protobuf::io::CodedInputStream input( &zcInputStream );
		
		protobuf_read( input, readValue );
	}
	
	return readValue;
}

TEST( Protobuf, IO )
{
	ASSERT_EQ( writeThenRead( 0 ), 		0 );
	ASSERT_EQ( writeThenRead( 123 ), 	123 );
	ASSERT_EQ( writeThenRead( 123U ), 	123U );
	ASSERT_EQ( writeThenRead( -123 ), 	-123 );
}

TEST( Protobuf, IO_Strings )
{
	std::string strTest = "test";
	ASSERT_EQ( writeThenRead( strTest ), "test" );
}

