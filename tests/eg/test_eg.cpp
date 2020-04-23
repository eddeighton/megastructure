


#include "egcomponent/egcomponent.hpp"


namespace megastructure
{
	
	class Test_EG : public EGComponent
	{
	public:
	
		~Test_EG()
		{
			
		}
	
		virtual void Initialise( MemorySystem* pMemorySystem, MegaProtocol* pMegaProtocol )
		{
		}
		
		virtual void Uninitialise()
		{
		}
		
		virtual void Cycle()
		{
		}
		
		virtual void Read( std::uint32_t uiDimensionType, std::uint32_t uiInstance, std::string& buffer )
		{
		}
		
		virtual void Write( std::uint32_t uiDimensionType, std::uint32_t uiInstance, const std::string& buffer )
		{
		}
		
		virtual void Invoke( std::uint32_t uiActionType, std::uint32_t uiInstance, const std::string& buffer )
		{
		}
		
		virtual void Pause( std::uint32_t uiActionType, std::uint32_t uiInstance )
		{
		}
		
		virtual void Resume( std::uint32_t uiActionType, std::uint32_t uiInstance )
		{
		}
		
		virtual void Stop( std::uint32_t uiActionType, std::uint32_t uiInstance )
		{
		}
	};

}
