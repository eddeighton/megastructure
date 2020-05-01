
#include "jobs.hpp"

namespace megastructure
{

		/*HMODULE hModule = LoadLibrary( args.componentPath.string().c_str() );
		VERIFY_RTE_MSG( hModule, "Failed to load module: " << args.componentPath );
		
		typedef megastructure::EGComponent*(*GetComponentFunctionPtr)();
		GetComponentFunctionPtr pFunction = (GetComponentFunctionPtr) GetProcAddress( hModule, "GET_EG_COMPONENT" );
		VERIFY_RTE_MSG( pFunction, "Failed to find GET_EG_COMPONENT in module: " << args.componentPath );
		
		megastructure::EGComponent* pComponent = (*pFunction)();*/
		
LoadProgramJob::LoadProgramJob( Component& component, 
	const std::string& strHostName, 
	const std::string& strProgramName )
		:	m_component( component ),
			m_strHostName( strHostName ),
			m_strProgramName( strProgramName )
{

}

void LoadProgramJob::run()
{
	std::cout << "Load Program Job run.  Hostname: " << m_strHostName << " Program Name: " << m_strProgramName << std::endl;
	
	m_component.jobComplete( shared_from_this() );
}


}