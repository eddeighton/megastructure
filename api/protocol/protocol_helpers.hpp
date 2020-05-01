
#ifndef PROTOCOL_HELPERS_16_APRIL_2020
#define PROTOCOL_HELPERS_16_APRIL_2020

#include "protocol/megastructure.pb.h"

#include "boost/filesystem/path.hpp"

#include <string>

namespace megastructure
{
	inline Message mss_enroll( bool bSuccess )
	{
		Message message;
		{
			Message::MSS_Enroll* pEnroll =
				message.mutable_mss_enroll();
			pEnroll->set_success( bSuccess );
		}
		return message;
	}
	
	inline Message mss_enroll( bool bSuccess, const std::string& strProgramName )
	{
		Message message;
		{
			Message::MSS_Enroll* pEnroll =
				message.mutable_mss_enroll();
			pEnroll->set_success( bSuccess );
			pEnroll->set_programname( strProgramName );
		}
		return message;
	}
	
	inline Message chs_enroll( bool bSuccess )
	{
		Message message;
		{
			Message::CHS_Enroll* pEnroll = message.mutable_chs_enroll();
			pEnroll->set_success( bSuccess );
		}
		return message;
	}
	inline Message chs_enroll( bool bSuccess, const boost::filesystem::path& workspacePath, const std::string& strSlaveName )
	{
		Message message;
		{
			Message::CHS_Enroll* pEnroll = message.mutable_chs_enroll();
			pEnroll->set_success( bSuccess );
			pEnroll->set_workspacepath( workspacePath.string() );
			pEnroll->set_slavename( strSlaveName );
		}
		return message;
	}
	
	inline Message sms_load( bool bSuccess )
	{
		Message response;
		{
			Message::SMS_Load* pLoadResponse =
				response.mutable_sms_load();
			pLoadResponse->set_success( bSuccess );
		}
		return response;
	}
}

#endif //PROTOCOL_HELPERS_16_APRIL_2020