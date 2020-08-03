

#include "activities.hpp"
#include "jobs.hpp"

#include "megastructure/mega.hpp"
#include "megastructure/log.hpp"

#include "protocol/protocol_helpers.hpp"

#include "common/assert_verify.hpp"

namespace megastructure
{
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

EnrollHostActivity::EnrollHostActivity( 
			Component& component,
			const std::string& hostprogram ) 
	:	m_component( component ),
		m_hostprogram( hostprogram ),
		m_strUnique( generateUniqueString() )
{
}

void EnrollHostActivity::start()
{
	Message message;
	{
		Message::HCQ_Enroll* pEnroll = message.mutable_hcq_enroll();
		pEnroll->set_processname( m_hostprogram );
		pEnroll->set_unique( m_strUnique );
	}
	m_component.send( message );
}

bool EnrollHostActivity::serverMessage( const Message& message )
{
	if( message.has_chs_enroll() )
	{
		const Message::CHS_Enroll& enroll = message.chs_enroll();
		
		if( enroll.success() )
		{
			m_strWorkspace = enroll.workspacepath();
			m_strSlaveName = enroll.slavename();
			
			{
				Message messageNext;
				{
					Message::HCQ_EnrollEG* pEnrolleg = messageNext.mutable_hcq_enrolleg();
					pEnrolleg->set_unique( m_strUnique );
				}
				m_component.sendeg( messageNext );
			}
		}
		else
		{
            SPDLOG_WARN( "Enroll failed" );
			m_component.activityComplete( shared_from_this() );
		}
		return true;
	}
	else if( message.has_chs_enrolleg() )
	{
		const Message::CHS_EnrollEG& enrolleg = message.chs_enrolleg();
		if( enrolleg.success() )
		{
			m_component.setSlaveWorkspacePath( m_strWorkspace );
			m_component.setSlaveName( m_strSlaveName );
            SPDLOG_INFO( "Enroll succeeded. Slave workspace: {} Slave name: {}",
                m_strWorkspace, m_strSlaveName );
		}
		else
		{
			std::cout << "Enroll failed" << std::endl;
		}
		m_component.activityComplete( shared_from_this() );
		return true;
	}
	return false;
}
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
bool LoadProgramActivity::serverMessage( const Message& message )
{
	if( message.has_chq_load() )
	{
		const Message::CHQ_Load& loadProgramRequest = message.chq_load();
		
		if( m_pLoadProgramJob )
		{
			//existing job active so deny
            SPDLOG_WARN( "Cannot load program: {} while already loading program : {}", 
                loadProgramRequest.programname(), m_pLoadProgramJob->getProgramName() );
			m_component.send( megastructure::hcs_load( false ) );
		}
		else
		{
			m_strProgramName 	= loadProgramRequest.programname();
			m_strHostName 		= loadProgramRequest.hostname();
            m_pLoadProgramJob.reset( new LoadProgramJob( m_component, 
                m_strHostName, m_strProgramName ) );
            m_component.startJob( m_pLoadProgramJob );
		}
			
		return true;
	}
	return false;
}
	
bool LoadProgramActivity::jobComplete( Job::Ptr pJob )
{
	if( m_pLoadProgramJob == pJob )
	{
        if( m_pLoadProgramJob->successful() )
        {
            SPDLOG_INFO( "Program: {} HostName: {} loaded successfully", m_strProgramName, m_strHostName );
            m_component.send( megastructure::hcs_load( true ) );
        }
        else
        {
            SPDLOG_WARN( "Program: {} HostName: {} failed to load", m_strProgramName, m_strHostName );
            m_component.send( megastructure::hcs_load( false ) );
        }
		m_pLoadProgramJob.reset();
		return true;
	}
	return false;
}
	
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
void BufferActivity::start()
{
    SPDLOG_TRACE( "Sending buffer request for: {} size: {}", m_bufferName, m_bufferSize );
    Message message;
    {
        Message::HCQ_Buffer* pBufferRequest = message.mutable_hcq_buffer();
        pBufferRequest->set_buffername( m_bufferName );
        pBufferRequest->set_size( m_bufferSize );
    }
    m_component.send( message );
}

bool BufferActivity::serverMessage( const Message& message )
{
	if( message.has_chs_buffer() )
	{
		const Message::CHS_Buffer& bufferResponse = message.chs_buffer();
        if( bufferResponse.buffername() == m_bufferName )
        {
            SPDLOG_TRACE( "Got buffer response for: {} size: {} shared name: {}", 
                m_bufferName, m_bufferSize, bufferResponse.sharedname() );
            m_result.set_value( bufferResponse.sharedname() );
			m_component.activityComplete( shared_from_this() );
            return true;
        }
    }
    return false;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void EGRequestManagerActivity::OnReadRequest( const Message::EG_Msg& egMsg )
{
    const std::uint32_t uiCycle 		            = egMsg.cycle();
    const Message::EG_Msg::Request& egRequest       = egMsg.request();
    const std::uint32_t uiCoordinator 	            = egRequest.coordinator();
    const std::uint32_t uiHost 			            = egRequest.host();
    const Message::EG_Msg::Request::Read& egRead    = egRequest.read();
    
    SimulationLock::Ptr pCurrentLock = m_component.getOrCreateCurrentLock();
    
    SimulationLock::CycleState currentCycleState;
    if( pCurrentLock->getHostCycleState( uiHost, currentCycleState ) )
    {
        switch( currentCycleState.state )
        {
            case SimulationLock::eReading     : 
            case SimulationLock::eWriting     : 
                {
                    const std::int32_t iType = egMsg.type();
                    const std::uint32_t uiInstance = egMsg.instance();
                    
                    if( std::shared_ptr< Program > pProgram = m_component.getProgram() )
                    {
                        std::string strBuffer;
                        pProgram->readRequest( iType, uiInstance, strBuffer );
                        
                        Message responseMessage;
                        {
                            Message::EG_Msg* pEGMsg = responseMessage.mutable_eg_msg();
                            pEGMsg->set_type( iType );
                            pEGMsg->set_instance( uiInstance );
                            pEGMsg->set_cycle( egMsg.cycle() );
                            
                            Message::EG_Msg::Response* pResponse = pEGMsg->mutable_response();
                            pResponse->set_coordinator( egRequest.coordinator() );
                            pResponse->set_host( egRequest.host() );
                            pResponse->set_value( strBuffer );
                        }
                        
                        SPDLOG_TRACE( "Sending eg read response type: {} instance: {} cycle: {} coordinator: {} host: {}", 
                            egMsg.type(), egMsg.instance(), egMsg.cycle(), egRequest.coordinator(), egRequest.host() );
                        
                        m_component.sendeg( responseMessage );
                    }
                    else
                    {
                        //error
                        megastructure::Message errorMessage;
                        {
                            megastructure::Message::EG_Msg* pErrorEGMsg = errorMessage.mutable_eg_msg();
                            pErrorEGMsg->set_type( egMsg.type() );
                            pErrorEGMsg->set_instance( egMsg.instance() );
                            pErrorEGMsg->set_cycle( egMsg.cycle() );
                            
                            megastructure::Message::EG_Msg::Error* pErrorEGMsgError = pErrorEGMsg->mutable_error();
                            pErrorEGMsgError->set_coordinator( egRequest.coordinator() );
                            pErrorEGMsgError->set_host( egRequest.host() );
                        }
                        SPDLOG_WARN( "Sending eg read response error type: {} instance: {} cycle: {} coordinator: {} host: {}", 
                            egMsg.type(), egMsg.instance(), egMsg.cycle(), egRequest.coordinator(), egRequest.host() );
                            
                        m_component.sendeg( errorMessage );
                    }
                }
                break;
            case SimulationLock::eRead        : 
            case SimulationLock::eWrite       : 
            case SimulationLock::eFinished    : 
            case SimulationLock::TOTAL_STATES : 
                THROW_RTE( "Invalid state for lock in read request" );
        }
    }
    else
    {
        SPDLOG_ERROR( "Read request when no current lock" );
        THROW_RTE( "Read request when no current lock" );
    }
}

void EGRequestManagerActivity::OnWriteRequest( const Message::EG_Msg& egMsg )
{
    const std::uint32_t uiCycle 		            = egMsg.cycle();
    const Message::EG_Msg::Request& egRequest       = egMsg.request();
    const std::uint32_t uiCoordinator 	            = egRequest.coordinator();
    const std::uint32_t uiHost 			            = egRequest.host();
    const Message::EG_Msg::Request::Write& egWrite  = egRequest.write();
    
    SimulationLock::Ptr pCurrentLock = m_component.getOrCreateCurrentLock();
    
    SimulationLock::CycleState currentCycleState;
    if( pCurrentLock->getHostCycleState( uiHost, currentCycleState ) )
    {
        switch( currentCycleState.state )
        {
            case SimulationLock::eWriting     : 
                {
                    const std::string& strBuffer = egWrite.value();
                    if( std::shared_ptr< Program > pProgram = m_component.getProgram() )
                    {
                        SPDLOG_TRACE( "Got write request size: {}", strBuffer.size() );
                        pProgram->writeRequest( strBuffer );
                    }
                    else
                    {
                        SPDLOG_WARN( "Attempting to write when program not loaded" );
                    }
                }
                break;
            case SimulationLock::eReading     : 
            case SimulationLock::eRead        : 
            case SimulationLock::eWrite       : 
            case SimulationLock::eFinished    : 
            case SimulationLock::TOTAL_STATES : 
                THROW_RTE( "Invalid state for lock in read request" );
        }
    }
    else
    {
        SPDLOG_ERROR( "Read request when no current lock" );
        THROW_RTE( "Read request when no current lock" );
    }
}

void EGRequestManagerActivity::OnLockRequest( const Message::EG_Msg& egMsg )
{
    const std::uint32_t uiCycle 		            = egMsg.cycle();
    const Message::EG_Msg::Request& egRequest       = egMsg.request();
    const std::uint32_t uiCoordinator 	            = egRequest.coordinator();
    const std::uint32_t uiHost 			            = egRequest.host();
    const Message::EG_Msg::Request::Lock& egLock    = egRequest.lock();
    
    if( egLock.read() )
    {
        SimulationLock::Ptr pCurrentLock = m_component.getOrCreateCurrentLock();
        SimulationLock::CycleState currentCycleState;
        if( pCurrentLock->getHostCycleState( uiHost, currentCycleState ) )
        {
            if( currentCycleState.uiCycle == uiCycle )
            {
                //already have a state when request read lock
                SPDLOG_ERROR( "Already have lock state when requesting read lock" );
                THROW_RTE( "Already have lock state when requesting read lock" );
            }
            else
            {
                //new lock has been requested so create future lock
                SimulationLock::Ptr pFutureLock = m_component.getOrCreateFutureLock();
                SimulationLock::CycleState futureCycleState;
                if( pFutureLock->getHostCycleState( uiHost, futureCycleState ) )
                {
                    SPDLOG_ERROR( "Already have future lock state" );
                    THROW_RTE( "Already have future lock state" );
                }
                else
                {
                    pFutureLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eRead );
                }
            }
        }
        else
        {
            //if currently reading then issue response immediately
            if( pCurrentLock->isReading() )
            {
                const SimulationLock::HostCycleState& hcs = 
                    pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eReading );
                issueLockResponse( hcs );
            }
            else
            {
                pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eRead );
            }
        }
    }
    else //write lock
    {
        SimulationLock::Ptr pCurrentLock = m_component.getOrCreateCurrentLock();
        
        SimulationLock::CycleState currentCycleState;
        if( pCurrentLock->getHostCycleState( uiHost, currentCycleState ) )
        {
            if( currentCycleState.uiCycle == uiCycle )
            {
                if( currentCycleState.state == SimulationLock::eRead )
                {
                    //upgrade to write lock
                    pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWrite );
                }
                else if( currentCycleState.state == SimulationLock::eReading )
                {
                    //upgrade to write lock
                    pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWrite );
                    
                    //if no longer reading - then start the writer lock immediately
                    if( !pCurrentLock->isReading() )
                    {
                        const SimulationLock::HostCycleState& hcs = 
                            pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWriting );
                        issueLockResponse( hcs );
                    }
                }
                else
                {
                    SPDLOG_ERROR( "Already have lock state when requesting write lock" );
                    THROW_RTE( "Already have lock state when requesting write lock" );
                }
            }
            else
            {
                //new lock has been requested so create future lock
                SimulationLock::Ptr pFutureLock = m_component.getOrCreateFutureLock();
                SimulationLock::CycleState futureCycleState;
                if( pFutureLock->getHostCycleState( uiHost, futureCycleState ) )
                {
                    if( ( futureCycleState.uiCycle == uiCycle ) && 
                        ( futureCycleState.state == SimulationLock::eRead ) )
                    {
                        //upgrade to future lock to write lock
                        pFutureLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWrite );
                    }
                    else
                    {
                        SPDLOG_ERROR( "Already have future write lock state" );
                        THROW_RTE( "Already have future write lock state" );
                    }
                }
                else
                {
                    pFutureLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWrite );
                }
            }
        }
        else
        {
            //start write lock
            pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eWrite );
        }
    }
}
void EGRequestManagerActivity::OnUnlockRequest( const Message::EG_Msg& egMsg )
{
    const std::uint32_t uiCycle 		                = egMsg.cycle();
    const Message::EG_Msg::Request& egRequest           = egMsg.request();
    const std::uint32_t uiCoordinator 	                = egRequest.coordinator();
    const std::uint32_t uiHost 			                = egRequest.host();
    const Message::EG_Msg::Request::Unlock& egUnLock    = egRequest.unlock();
    
    SimulationLock::Ptr pCurrentLock = m_component.getCurrentLock();
    VERIFY_RTE( pCurrentLock );
    
    SimulationLock::HostCycleStateVector& hostCycleStates = 
        pCurrentLock->getHostCycleStates();
    
    SimulationLock::CycleState oldCycleState;
    if( pCurrentLock->getHostCycleState( uiHost, oldCycleState ) )
    {
        pCurrentLock->setHostCycleState( uiCoordinator, uiHost, uiCycle, SimulationLock::eFinished );
        
        if( pCurrentLock->allFinished() )
        {
            //done - release the entire simulation lock
            m_component.releaseCurrentLock();
        }
        else if( !pCurrentLock->isReading() )
        {
            if( pCurrentLock->isRead() )
            {
                //start all readers
                for( SimulationLock::HostCycleState& hcs : hostCycleStates )
                {
                    if( hcs.cycleState.state == SimulationLock::eRead )
                    {                
                        hcs.cycleState.state = SimulationLock::eReading;
                        issueLockResponse( hcs );
                    }
                }
            }
            else
            {
                //attempt to start a writer
                bool bFoundFirst = false;
                for( SimulationLock::HostCycleState& hcs : hostCycleStates )
                {
                    if( hcs.cycleState.state == SimulationLock::eWrite )
                    {
                        hcs.cycleState.state = SimulationLock::eWriting; 
                        issueLockResponse( hcs );
                        bFoundFirst = true;
                        break;
                    }
                }
                VERIFY_RTE( bFoundFirst );
            }
        }
    }
    else
    {
        SPDLOG_ERROR( "Received unlock from host with no lock" );
        THROW_RTE( "Received unlock from host with no lock" );
    } 
}


bool EGRequestManagerActivity::serverMessage( const Message& message )
{
	if( message.has_eg_msg() )
	{
		const Message::EG_Msg& egMsg = message.eg_msg();
		if( egMsg.has_request() )
		{
			const Message::EG_Msg::Request& egRequest = egMsg.request();
			
            if( egRequest.has_read() )
            {
                OnReadRequest( egMsg );
            }
            else if( egRequest.has_write() )
            {
                OnWriteRequest( egMsg );
            }
            else if( egRequest.has_lock() )
            {
                OnLockRequest( egMsg );
            }
            else if( egRequest.has_unlock() )
            {
                OnUnlockRequest( egMsg );
            }
            else 
            {
                SPDLOG_ERROR( "Unknown request type" );
                THROW_RTE( "Unknown request type" );
            }
        }
		
		return true;
	}
	return false;
}


bool EGRequestManagerActivity::activityComplete( Activity::Ptr pActivity )
{
	return false;
}

void EGRequestManagerActivity::simulationLockStarted()
{
    SimulationLock::Ptr pCurrentLock = m_component.getCurrentLock();
    VERIFY_RTE( pCurrentLock );
    
    SimulationLock::HostCycleStateVector& hostCycleStates = 
        pCurrentLock->getHostCycleStates();
        
    if( pCurrentLock->isRead() )
    {
        //start all readers
        for( SimulationLock::HostCycleState& hcs : hostCycleStates )
        {
            if( hcs.cycleState.state == SimulationLock::eRead )
            {                
                hcs.cycleState.state = SimulationLock::eReading;
                issueLockResponse( hcs );
            }
        }
    }
    else
    {
        //start first writer
        bool bFoundFirst = false;
        for( SimulationLock::HostCycleState& hcs : hostCycleStates )
        {
            if( hcs.cycleState.state == SimulationLock::eWrite )
            {
                hcs.cycleState.state = SimulationLock::eWriting; 
                issueLockResponse( hcs );
                bFoundFirst = true;
                break;
            }
        }
        VERIFY_RTE( bFoundFirst );
    }
}

void EGRequestManagerActivity::issueLockResponse( const SimulationLock::HostCycleState& hcs ) const
{
    Message responseMessage;
    {
        Message::EG_Msg* pEGMsg = responseMessage.mutable_eg_msg();
        Message::EG_Msg::Response* pResponse = pEGMsg->mutable_response();
        pResponse->set_coordinator( hcs.uiCoordinator );
        pResponse->set_host( hcs.uiHost );
        
        //all importand - get the current clock cycle for THIS component
        pEGMsg->set_cycle( m_component.getCurrentCycle() );
    }
    SPDLOG_TRACE( "Sending eg lock response coordinator: {} host: {}", 
        hcs.uiCoordinator, hcs.uiHost );
    m_component.sendeg( responseMessage );
}

}