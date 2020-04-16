
#ifndef PROTOCOL_HELPERS_16_APRIL_2020
#define PROTOCOL_HELPERS_16_APRIL_2020

#include "protocol/megastructure.pb.h"

namespace megastructure
{
	
	
	inline Message mss_enroll( bool bSuccess )
	{
		Message message;
		{
			Message::MSS_Enroll* pEnroll =
				message.mutable_mss_enroll();
			pEnroll->set_success( false );
		}
		return message;
	}
	
	
}

#endif //PROTOCOL_HELPERS_16_APRIL_2020