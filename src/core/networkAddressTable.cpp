
#include "megastructure/networkAddressTable.hpp"

#include "schema/projectTree.hpp"

#include "eg_compiler/sessions/implementation_session.hpp"

namespace megastructure
{

using ClientIDMap = std::map< eg::IndexedObject::Index, std::uint32_t >;

void recurseClient( const eg::concrete::Action* pAction, std::uint32_t uiClientID, ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pAction->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		clientIDMap.insert( std::make_pair( pElement->getIndex(), uiClientID ) );
			
		if( const eg::concrete::Action* pNestedAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			recurseClient( pNestedAction, uiClientID, clientIDMap );
		}
	}
}

void recurseMappedClients( const eg::concrete::Action* pRoot, const NetworkAddressTable::ClientMap& clientMap, ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pClientAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			const std::string& strActionIdentifier = 
				pClientAction->getAction()->getIdentifier();
			
			NetworkAddressTable::ClientMap::const_iterator iFind = 
				clientMap.find( strActionIdentifier );
			if( iFind != clientMap.end() )
			{
				const std::uint32_t uiClientID = iFind->second;
				recurseClient( pClientAction, uiClientID, clientIDMap );
			}
			else
			{
				//THROW_RTE( "Could not map client action: " << pClientAction->getName() );
			}
		}
	}
}

void recurseSlave( const eg::concrete::Action* pRoot, 
	const NetworkAddressTable::ClientMap& clientMap,
	const std::string& strCoordinatorName, 
	ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pCoordinatorAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			const std::string& strActionIdentifier = 
				pCoordinatorAction->getAction()->getIdentifier();
			if( strCoordinatorName == strActionIdentifier )
			{
				recurseMappedClients( pCoordinatorAction, clientMap, clientIDMap );
			}
			else
			{
				recurseClient( pCoordinatorAction, NetworkAddressTable::MasterID, clientIDMap );
			}
		}
	}
}


NetworkAddressTable::NetworkAddressTable( 
	const ClientMap& clients, 
	std::shared_ptr< ProjectTree > pProgramTree )
{
	const boost::filesystem::path programDatabasePath = 
		pProgramTree->getAnalysisFileName();
	
    if( boost::filesystem::exists( programDatabasePath ) )
    {
		eg::ReadSession session( programDatabasePath );
		
		ClientIDMap clientIDMap;
		
		const eg::concrete::Action* pRoot = session.getInstanceRoot();
		
		recurseMappedClients( pRoot, clients, clientIDMap );
		
		int iHighest = 0;
		for( auto& i : clientIDMap )
		{
			if( i.first > iHighest )
				iHighest = i.first;
		}
		
		m_table.resize( iHighest );
		
		for( auto& i : clientIDMap )
		{
			m_table[ i.first ] = i.second;
		}
	}
}

NetworkAddressTable::NetworkAddressTable( 
	const ClientMap& clients, 
	const std::string& strCoordinatorName, 
	std::shared_ptr< ProjectTree > pProgramTree )
{
	const boost::filesystem::path programDatabasePath = 
		pProgramTree->getAnalysisFileName();

    if( boost::filesystem::exists( programDatabasePath ) )
    {
	    eg::ReadSession session( programDatabasePath );
	
	    ClientIDMap clientIDMap;
	
	    const eg::concrete::Action* pRoot = session.getInstanceRoot();
	
	    recurseSlave( pRoot, clients, strCoordinatorName, clientIDMap );
	
	    int iHighest = 0;
	    for( auto& i : clientIDMap )
	    {
		    if( i.first > iHighest )
			    iHighest = i.first;
	    }
	
	    m_table.resize( iHighest + 1 );
	
	    for( auto& i : clientIDMap )
	    {
		    m_table[ i.first ] = i.second;
	    }
    }
}


}