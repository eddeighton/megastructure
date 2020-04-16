
#ifndef CLIENTMAP_16_APRIL_2020
#define CLIENTMAP_16_APRIL_2020

#include <map>

namespace megastructure
{

class ClientMap
{
public:
	using ClientIDMap = std::map< std::string, std::uint32_t >;
	
	template< typename T >
	inline void forEachClientID( T& functor )
	{
		for( ClientIDMap::iterator 
			i = m_clients.begin(), iEnd = m_clients.end();
			i!=iEnd; )
		{
			functor( i->second );
		}
	}

	inline bool getClientID( const std::string& strName, std::uint32_t& clientID ) const
	{
		ClientIDMap::const_iterator iFind = m_clients.find( strName );
		if( iFind != m_clients.end() )
		{
			clientID = iFind->second;
			return true;
		}
		else
		{
			return false;
		}
	}
	
	inline bool enroll( const std::string& strName, std::uint32_t clientID )
	{
		std::uint32_t existingClientID;
		if( !getClientID( strName, existingClientID ) )
		{
			m_clients.insert( std::make_pair( strName, clientID ) );
			return true;
		}
		else
		{
			return false;
		}
	}
	
	inline void removeClient( std::uint32_t clientID )
	{
		for( ClientIDMap::iterator 
			i = m_clients.begin(), iEnd = m_clients.end();
			i!=iEnd; )
		{
			if( i->second == clientID )
			{
				i = m_clients.erase( i );
			}
			else
			{
				++i;
			}
		}
	}
	
	const ClientIDMap& getClients() const { return m_clients; }
	
private:
	ClientIDMap m_clients;
};

}

#endif //CLIENTMAP_16_APRIL_2020