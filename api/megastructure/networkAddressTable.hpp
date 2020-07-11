
#ifndef NETWORK_ADDRESS_TABLE_02_MAY_2020
#define NETWORK_ADDRESS_TABLE_02_MAY_2020


#include <map>
#include <vector>
#include <cstdint>
#include <memory>
#include <limits>

class ProjectTree;

namespace megastructure
{

class NetworkAddressTable
{
public:
	using ClientMap = std::map< std::string, std::uint32_t >;
	using Table = std::vector< std::uint32_t >;
	using Ptr = std::shared_ptr< NetworkAddressTable >;
	
	static const std::uint32_t UnMapped     = 0; //default value
	static const std::uint32_t MasterID     = std::numeric_limits< std::uint32_t >::max();
	static const std::uint32_t SelfID       = std::numeric_limits< std::uint32_t >::max() - 1;
	
	NetworkAddressTable( const ClientMap& clients, 
		std::shared_ptr< ProjectTree > pProgramTree );
		
	NetworkAddressTable( const ClientMap& clients, 
		const std::string& strCoordinatorName,
		std::shared_ptr< ProjectTree > pProgramTree );
		
	NetworkAddressTable( const std::string& strCoordinatorName, 
		const std::string& strHostName,
		std::shared_ptr< ProjectTree > pProgramTree );
	
	inline std::uint32_t getClientForType( std::uint32_t uiType ) const
	{
		return m_table[ uiType ];
	}

	
private:
	Table m_table;
};

class ProgramTypeTable
{
public:
	using Table = std::vector< std::uint32_t >;
	using Ptr = std::shared_ptr< ProgramTypeTable >;
	
	ProgramTypeTable( std::shared_ptr< ProjectTree > pProgramTree );
	
	inline std::uint32_t getProgramForType( std::uint32_t uiType ) const
	{
		return m_table[ uiType ];
	}
private:
	Table m_table;
};

}

#endif //NETWORK_ADDRESS_TABLE_02_MAY_2020