
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
				pClientAction->getContext()->getIdentifier();
			
			NetworkAddressTable::ClientMap::const_iterator iFind = 
				clientMap.find( strActionIdentifier );
			if( iFind != clientMap.end() )
			{
				const std::uint32_t uiClientID = iFind->second;
				recurseClient( pClientAction, uiClientID, clientIDMap );
			}
			else
			{
				recurseClient( pClientAction, NetworkAddressTable::UnMapped, clientIDMap );
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
				pCoordinatorAction->getContext()->getIdentifier();
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

void recurseHost( const eg::concrete::Action* pRoot, 
		const std::string& strHostName, 
		ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pHostAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			const std::string& strActionIdentifier = 
				pHostAction->getContext()->getIdentifier();
			if( strHostName == strActionIdentifier )
			{
				recurseClient( pHostAction, NetworkAddressTable::SelfID, clientIDMap );
			}
			else
			{
				recurseClient( pHostAction, NetworkAddressTable::MasterID, clientIDMap );
			}
		}
	}
}

void recurseHost( const eg::concrete::Action* pRoot, 
		const std::string& strCoordinatorName, 
		const std::string& strHostName, 
		ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pCoordinatorAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			const std::string& strActionIdentifier = 
				pCoordinatorAction->getContext()->getIdentifier();
			if( strCoordinatorName == strActionIdentifier )
			{
				recurseHost( pCoordinatorAction, strHostName, clientIDMap );
			}
			else
			{
				recurseClient( pCoordinatorAction, NetworkAddressTable::MasterID, clientIDMap );
			}
		}
	}
}


void buildTable( const ClientIDMap& clientIDMap, NetworkAddressTable::Table& table )
{
	int iHighest = 0;
	for( auto& i : clientIDMap )
	{
		if( i.first > iHighest )
			iHighest = i.first;
	}
	
	table.resize( iHighest + 1 );
	
	for( auto& i : clientIDMap )
	{
		table[ i.first ] = i.second;
	}
}

const eg::concrete::Action* getMegaRoot( const eg::concrete::Action* pInstanceRoot )
{
    const eg::concrete::Action* pMegaRootResult = nullptr;

	const std::vector< eg::concrete::Element* >& children = pInstanceRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pMegaRoot = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
            VERIFY_RTE( !pMegaRootResult );
            pMegaRootResult = pMegaRoot;
		}
	}

    VERIFY_RTE_MSG( pMegaRootResult, "Failed to locate mega root" );

    return pMegaRootResult;
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
		recurseMappedClients( getMegaRoot( session.getInstanceRoot() ), clients, clientIDMap );
		buildTable( clientIDMap, m_table );
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
	    recurseSlave( getMegaRoot( session.getInstanceRoot() ), clients, strCoordinatorName, clientIDMap );
		buildTable( clientIDMap, m_table );
    }
}

		
NetworkAddressTable::NetworkAddressTable( const std::string& strCoordinatorName, 
							const std::string& strHostName, 
							std::shared_ptr< ProjectTree > pProgramTree )
{
	const boost::filesystem::path programDatabasePath = 
		pProgramTree->getAnalysisFileName();

    if( boost::filesystem::exists( programDatabasePath ) )
    {
	    eg::ReadSession session( programDatabasePath );
	    ClientIDMap clientIDMap;
	    recurseHost( getMegaRoot( session.getInstanceRoot() ), strCoordinatorName, strHostName, clientIDMap );
		buildTable( clientIDMap, m_table );
    }
}


void recurseTypes( const eg::concrete::Action* pRoot, ClientIDMap& clientIDMap )
{
	const std::vector< eg::concrete::Element* >& children = pRoot->getChildren();
	for( const eg::concrete::Element* pElement : children )
	{
		if( const eg::concrete::Action* pAction = 
			dynamic_cast< const eg::concrete::Action* >( pElement ) )
		{
			bool bIsProgramAction = false;
			if( const eg::interface::Root* pInterfaceRoot = 
				dynamic_cast< const eg::interface::Root* >( pAction->getContext() ) )
			{
				if( pInterfaceRoot->getRootType() == eg::eHostName )
				{
					bIsProgramAction = true;
				}
			}
			
			if( bIsProgramAction )
			{
				recurseClient( pAction, pAction->getIndex(), clientIDMap );
			}
			else
			{
				recurseTypes( pAction, clientIDMap );
			}
		}
	}
}


ProgramTypeTable::ProgramTypeTable( std::shared_ptr< ProjectTree > pProgramTree )
{
	const boost::filesystem::path programDatabasePath = 
		pProgramTree->getAnalysisFileName();

    if( boost::filesystem::exists( programDatabasePath ) )
    {
	    eg::ReadSession session( programDatabasePath );
	    ClientIDMap clientIDMap;
	    recurseTypes( getMegaRoot( session.getInstanceRoot() ), clientIDMap );
		buildTable( clientIDMap, m_table );
    }
}
		
}