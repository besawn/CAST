/*================================================================================

    csmd/src/daemon/include/csm_network_event.h

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_CSM_NETWORK_EVENT_H_
#define CSMD_SRC_DAEMON_INCLUDE_CSM_NETWORK_EVENT_H_

#include "csmnet/src/CPP/csm_message_and_address.h"

namespace csm {
namespace daemon {

typedef CoreEventBase<csm::network::MessageAndAddress> NetworkEvent;
typedef std::deque<const CoreEvent*> NetworkEventQueue;


}   // namespace daemon
}  // namespace csm

#endif /* CSMD_SRC_DAEMON_INCLUDE_CSM_NETWORK_EVENT_H_ */
