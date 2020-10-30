
#include <gtest/gtest.h>

inline bool parseIdentifier( const std::string& identifier, std::string& strID, int& index )
{
    if( identifier.size() > 5 )
    {
        if( identifier[ identifier.size() - 5U ] == '_' )
        {
            if( std::isdigit( identifier[ identifier.size() - 1U ] ) &&
                std::isdigit( identifier[ identifier.size() - 2U ] ) &&
                std::isdigit( identifier[ identifier.size() - 3U ] ) &&
                std::isdigit( identifier[ identifier.size() - 4U ] ) )
            {
                strID.assign( identifier.begin(), identifier.begin() + identifier.size() - 5U );
                index = stoi( identifier.substr( identifier.size() - 4U, 4U ) );
                return true;
            }
        }
    }
    return false;
}

inline std::string makeIdentifier( const std::string& identifier, int index )
{
    std::ostringstream os;
    os << identifier << '_' << std::setw( 4 ) << std::setfill( '0' ) << index;
    return os.str();
}

TEST( MakeIdentifierTests, IdentifierParserTests )
{
    
    
    ASSERT_EQ( makeIdentifier( std::string( "test" ), 0 ), std::string( "test_0000" ) );
    ASSERT_EQ( makeIdentifier( std::string( "test" ), 1 ), std::string( "test_0001" ) );
    ASSERT_EQ( makeIdentifier( std::string( "test" ), 12 ), std::string( "test_0012" ) );
    ASSERT_EQ( makeIdentifier( std::string( "test" ), 123 ), std::string( "test_0123" ) );
    ASSERT_EQ( makeIdentifier( std::string( "test" ), 1234 ), std::string( "test_1234" ) );

}


TEST( ParseIdentifierTests, IdentifierParserTests )
{
    {
        std::string str;
        int index;
        ASSERT_TRUE( parseIdentifier( std::string( "test_0000" ), str, index ) );
        ASSERT_EQ( str, std::string( "test" ) );
        ASSERT_EQ( index, 0 );
    }
    {
        std::string str;
        int index;
        ASSERT_TRUE( parseIdentifier( std::string( "test_0001" ), str, index ) );
        ASSERT_EQ( str, std::string( "test" ) );
        ASSERT_EQ( index, 1 );
    }
    {
        std::string str;
        int index;
        ASSERT_TRUE( parseIdentifier( std::string( "test_0012" ), str, index ) );
        ASSERT_EQ( str, std::string( "test" ) );
        ASSERT_EQ( index, 12 );
    }
    {
        std::string str;
        int index;
        ASSERT_TRUE( parseIdentifier( std::string( "test_0123" ), str, index ) );
        ASSERT_EQ( str, std::string( "test" ) );
        ASSERT_EQ( index, 123 );
    }
    {
        std::string str;
        int index;
        ASSERT_TRUE( parseIdentifier( std::string( "test_1234" ), str, index ) );
        ASSERT_EQ( str, std::string( "test" ) );
        ASSERT_EQ( index, 1234 );
    }

}
