
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
	inline Message chs_enrolleg( bool bSuccess )
	{
		Message message;
		{
			Message::CHS_EnrollEG* pEnrolleg = message.mutable_chs_enrolleg();
			pEnrolleg->set_success( bSuccess );
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
    
    
	inline Message chs_buffer( const std::string& strBufferName, const std::string& strSharedName )
	{
		Message response;
		{
			Message::CHS_Buffer* pBufferResponse =
				response.mutable_chs_buffer();
			pBufferResponse->set_buffername( strBufferName );
			pBufferResponse->set_sharedname( strSharedName );
		}
		return response;
	}
    
	
	inline Message hcs_load( bool bSuccess )
	{
		Message response;
		{
			Message::HCS_Load* pLoadResponse =
				response.mutable_hcs_load();
			pLoadResponse->set_success( bSuccess );
		}
		return response;
	}
    
}

#endif //PROTOCOL_HELPERS_16_APRIL_2020