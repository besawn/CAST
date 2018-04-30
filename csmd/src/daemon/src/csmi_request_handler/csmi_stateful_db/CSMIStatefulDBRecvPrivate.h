/*================================================================================

    csmd/src/daemon/src/csmi_request_handler/csmi_stateful_db/CSMIStatefulDBRecvPrivate.h

  © Copyright IBM Corporation 2015,2016. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.
 
================================================================================*/
#ifndef CSMIStatefulDBRecvPrivate_h
#define CSMIStatefulDBRecvPrivate_h

#include "../csmi_stateful_db.h"
#include "../csmi_db_resp_state.h"

#define STATE_NAME "StatefulDBRecvPrivate"
/** @brief 
 *
 * @param state_name   
 * @param target_state 
 * @param fail_state   
 */
class  StatefulDBRecvPrivate : public DBRespState
{
private:
    CSMIStatefulDB *_Handler;

public:
    StatefulDBRecvPrivate(
            CSMIStatefulDB *handler,
            uint64_t success,
            uint64_t failure,
            uint64_t final_state,
            uint64_t timeout = csm::daemon::helper::BAD_STATE,
            uint64_t timeout_length = csm::daemon::helper::BAD_TIMEOUT_LEN,
            uint64_t alternate = csm::daemon::helper::BAD_STATE):
        DBRespState(success, failure, final_state, timeout, 
            timeout_length, alternate, STATE_NAME),
        _Handler(handler) { }

protected:
    virtual bool HandleDBResp(
        const std::vector<csm::db::DBTuple *>& tuples,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        csm::daemon::EventContextHandlerState_sptr ctx ) final;


    /** @brief See @ref CSMIHandlerState::DefaultHandleError for documentation. */
    virtual void HandleError(
        csm::daemon::EventContextHandlerState_sptr ctx,
        const csm::daemon::CoreEvent &aEvent,
        std::vector<csm::daemon::CoreEvent*>& postEventList,
        bool byAggregator = false) final
    { 
        CSMIHandlerState::DefaultHandleError( ctx, aEvent, postEventList, byAggregator );
    } 

    /** @brief Unused. */
    virtual void GenerateTimeoutResponse(
        csm::daemon::EventContextHandlerState_sptr ctx,
        std::vector<csm::daemon::CoreEvent*>& postEventList ) final { }
};
#undef STATE_NAME

#endif
