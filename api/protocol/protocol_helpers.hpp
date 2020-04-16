
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
	
	
	inline Message chs_enroll( bool bSuccess )
	{
		Message message;
		{
			Message::CHS_Enroll* pEnroll = message.mutable_chs_enroll();
			pEnroll->set_success( bSuccess );
			pEnroll->set_hostpath( "" );
			pEnroll->set_program( "" );
		}
		return message;
	}
	inline Message chs_enroll( bool bSuccess, const std::string& strHostPath, const std::string& strProgram )
	{
		Message message;
		{
			Message::CHS_Enroll* pEnroll = message.mutable_chs_enroll();
			pEnroll->set_success( bSuccess );
			pEnroll->set_hostpath( strHostPath );
			pEnroll->set_program( strProgram );
		}
		return message;
	}
	
	
}

#endif //PROTOCOL_HELPERS_16_APRIL_2020