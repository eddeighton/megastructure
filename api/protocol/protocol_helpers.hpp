
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
	
    
    enum ConfigActivityType
    {
        eConfigLoading,
        eConfigSaving,
        TOTAL_CONFIG_ACTIVITY_TYPES
    };
    
    inline Message config_load_msq()
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Load* m2 = m1->mutable_load();
            Message::Config::Load::MSQ* m3 = m2->mutable_msq();
        }
        return msg;
    }
    
    inline Message config_save_msq()
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Save* m2 = m1->mutable_save();
            Message::Config::Save::MSQ* m3 = m2->mutable_msq();
        }
        return msg;
    }
    
    inline Message config_load_sms( bool bSuccess )
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Load* m2 = m1->mutable_load();
            Message::Config::Load::SMS* m3 = m2->mutable_sms();
            m3->set_success( bSuccess );
        }
        return msg;
    }
    
    inline Message config_save_sms( bool bSuccess )
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Save* m2 = m1->mutable_save();
            Message::Config::Save::SMS* m3 = m2->mutable_sms();
            m3->set_success( bSuccess );
        }
        return msg;
    }
    
    inline bool config_load_sms( const Message::Config& configMsg )
    {
        if( configMsg.has_load() )
        {
            const Message::Config::Load& m = configMsg.load();
            if( m.has_sms() )
            {
                const Message::Config::Load::SMS& sms = m.sms();
                return sms.success();
            }
        }
        return false;
    }
    
    inline bool config_save_sms( const Message::Config& configMsg )
    {
        if( configMsg.has_save() )
        {
            const Message::Config::Save& m = configMsg.save();
            if( m.has_sms() )
            {
                const Message::Config::Save::SMS& sms = m.sms();
                return sms.success();
            }
        }
        return false;
    }
    
    inline Message config_load_chq()
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Load* m2 = m1->mutable_load();
            Message::Config::Load::CHQ* m3 = m2->mutable_chq();
        }
        return msg;
    }
    
    inline bool config_load_chq( const Message::Config& configMsg )
    {
        if( configMsg.has_load() )
        {
            const Message::Config::Load& m = configMsg.load();
            return m.has_chq();
        }
        return false;
    }
    
    inline Message config_save_chq()
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Save* m2 = m1->mutable_save();
            Message::Config::Save::CHQ* m3 = m2->mutable_chq();
        }
        return msg;
    }
    
    inline bool config_save_chq( const Message::Config& configMsg )
    {
        if( configMsg.has_save() )
        {
            const Message::Config::Save& m = configMsg.save();
            return m.has_chq();
        }
        return false;
    }
    
    inline Message config_load_hcs( bool bSuccess )
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Load* m2 = m1->mutable_load();
            Message::Config::Load::HCS* m3 = m2->mutable_hcs();
            m3->set_success( bSuccess );
        }
        return msg;
    }
    
    inline Message config_save_hcs( bool bSuccess )
    {
		Message msg;
		{
            Message::Config* m1 = msg.mutable_config_msg();
            Message::Config::Save* m2 = m1->mutable_save();
            Message::Config::Save::HCS* m3 = m2->mutable_hcs();
            m3->set_success( bSuccess );
        }
        return msg;
    }
}

#endif //PROTOCOL_HELPERS_16_APRIL_2020