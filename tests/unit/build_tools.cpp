
#include "common/file.hpp"
#include "common/assert_verify.hpp"

#include <gtest/gtest.h>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>

#include <map>
#include <string>
#include <utility>
#include <iostream>
#include <sstream>

using HashCode = std::string;
using FileHash = std::pair< HashCode, boost::filesystem::path >;
using Manifest = std::map< FileHash, boost::filesystem::path >;

void load( std::istream& inStream, Manifest& output )
{
    std::string strLine;
    while( std::getline( inStream, strLine ) )
    {
        using Tokeniser = boost::tokenizer< boost::char_separator< char > >;
        boost::char_separator< char > sep( "," );
        Tokeniser tokens( strLine, sep );
        for ( Tokeniser::iterator i = tokens.begin(); i != tokens.end(); ++i )
        {
            FileHash fileHash;
            fileHash.first = *i;
            
            if( ++i == tokens.end() )
                THROW_RTE( "Error in stash manifest" );
            fileHash.second = *i;
            
            if( ++i == tokens.end() )
                THROW_RTE( "Error in stash manifest" );
            
            output.insert( std::make_pair( fileHash, *i ) );
        }
    }
}

void save( const Manifest& input, std::ostream& outStream )
{
    bool bFirst = true;
    for( Manifest::const_iterator i = input.begin(),
        iEnd = input.end(); i!=iEnd; ++i )
    {
        if( bFirst ) bFirst = false;
        else outStream << '\n';
        outStream << i->first.first << ',' << i->first.second.string() << ',' << i->second.string();
    }
}

void test( int expectedEntryCount, const std::string& str )
{
    std::istringstream inStream( str );
    
    Manifest manifest;
    load( inStream, manifest );
    
    std::ostringstream outStream;
    save( manifest, outStream );
    
    ASSERT_EQ( expectedEntryCount, manifest.size() );
    ASSERT_EQ( str, outStream.str() );
}
    
TEST( BuildTools, Manifest1 )
{
    test( 0, "" );
}

TEST( BuildTools, Manifest2 )
{
    test( 1, "123abc,w:\\somepath/that has spaces\\and stuff.thing,file_123.stash" );
}

TEST( BuildTools, Manifest3 )
{
    test( 2, "123abc,w:\\somepath/that has spaces\\and stuff.thing,file_123.stash\n223abc,w:\\somepath/that has spaces\\and stuff.thing,file_123.stash" );
}

TEST( BuildTools, Manifest4 )
{
    test( 3, "123abc,w:\\somepath/that has spaces\\and stuff.thing,file_123.stash\n123abc,w:\\somepath/that has spaces2\\and stuff.thing,file_123.stash\n323abc,w:\\somepath/that has spaces\\and stuff.thing,file_123.stash" );
}