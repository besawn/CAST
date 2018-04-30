/*================================================================================

    csmi/src/common/src/csm_api_common.c

  © Copyright IBM Corporation 2015-2017. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>

#include "csmnet/src/C/csm_network_internal_api_c.h"
#include "csmutil/include/csmutil_logging.h"

#include "csmi/src/common/include/csmi_api_internal.h"
#include "csmi/src/common/include/csmi_serialization.h"
#include "csmi/src/common/include/csmi_common_utils.h"

#include "csmi/include/csmi_type_common.h"

#include <string.h>

static volatile int initialized = 0;
static csm_net_endpoint_t *ep=NULL;
static pthread_mutex_t disconnect_lock;

static int on_disconnect( csm_net_msg_t *aMsg )
{
  csmutil_logging(error, "Terminating CSMI lib because of daemon disconnect.\n" );
  int rc = 0;
  if( initialized )
    rc = csm_term_lib();
  return rc;
}

/**
 * \brief CSM API library initialization
 *
 * Initializes the library by creating sockets and connects to the local daemon
 *
 * @param[in]   version_id The version this initialization should be performed as.
 *
 * @return  0 on success, error code otherwise
 *
 * \note environment variables that impact behavior:
 *
 *   CSM_LOGLEVEL   allows to control the amount of logging (default: info)
 *   CSM_SSOCKET    set the socket path of the daemon (default: /run/csmd.sock)
 *
 * \todo library is not fork or thread safe yet, the caller is responsible to prevent
 *       shared use of the calls
 */
int csm_init_lib_vers(int64_t version_id)
{
    // TODO min version check.
  struct timeval tv;
  uint64_t init_msgId;
  int rc = 0;
  char *socket_name = CSM_NETWORK_LOCAL_SSOCKET;
  csmutil_logging(debug, "Socket Name %s", socket_name);
  
  char *csm_log_env = NULL;
  if ( (csm_log_env = getenv("CSM_LOGLEVEL")) )
  {
    csmutil_logging_level_set(csm_log_env);
  }
  csmutil_logging(debug, "Log Level retrieved");

  pthread_mutex_lock( &disconnect_lock );
  if (initialized) {
    csmutil_logging(warning, "csm_init_lib() already called");
    pthread_mutex_unlock( &disconnect_lock );
    return 0;
  }
  csmutil_logging(debug, "Not initialized");

  if ( (ep = csm_net_unix_Init(0)) == NULL) {
    perror("csm_net_unit_Init");
    csmutil_logging(error, "csm_net_unix_Init() returned NULL");
    pthread_mutex_unlock( &disconnect_lock );
    return EBADFD;
  }
  csmutil_logging(debug, "Net inited");
  
  if (getenv("CSM_SSOCKET")) socket_name = getenv("CSM_SSOCKET");
  rc = csm_net_unix_Connect(ep, socket_name);
  if ( rc != 0) {
    perror("csm_net_unix_Connect");
    csmutil_logging(error, "csm_net_unix_Connect() failed: %s", socket_name);
    csm_net_unix_Exit(ep);
    pthread_mutex_unlock( &disconnect_lock );
    return rc;
  }

  csmutil_logging(debug, "Before hdl");
  csmi_cmd_hdl_init();

  gettimeofday(&tv, NULL);
  srand(tv.tv_sec + tv.tv_usec);
  init_msgId = (uint64_t)rand();
  csmi_set_init_msgId(init_msgId);

  ep->_on_disconnect = on_disconnect;
  
  initialized = 1;

  csmutil_logging(trace, "csm_init_lib initialized (init_msg_id=%d)", init_msgId);

  pthread_mutex_unlock( &disconnect_lock );
  return 0;
}

/**
 * \brief Terminate the library
 *
 * disconnects from the daemon and destroys the sockets
 *
 * @param  none
 *
 * @return 0 on success or non-zero in case of errors
 *
 */
int csm_term_lib()
{
  pthread_mutex_lock( &disconnect_lock );
  int rc = 0;
  if (!initialized) {
    csmutil_logging(warning, "not initialized or already terminated\n");
    pthread_mutex_unlock( &disconnect_lock );
    return ENOTCONN;
  }
  
  rc = csm_net_unix_Exit(ep);

  csmutil_logging(trace, "csm_term_lib() done");
  initialized = 0;
  pthread_mutex_unlock( &disconnect_lock );
  return rc;
}

/**
 * \brief Send a message to the daemon
 *
 * @param[in]   msg   pointer to message structure
 *
 * @return    number of bytes sent or negative error code
 *
 */
int csmi_net_unix_Send(csm_net_msg_t *msg)
{
    int rc;

    int retry = 5;
    while(( ! initialized ) && ( --retry > 0))
    {
      csmutil_logging(warning, "%s-%d: csmi_init_lib() not initialized. Trying...\n", __FILE__, __LINE__);
      csm_init_lib();
    }

    if (!initialized) {
      csmutil_logging(warning, "%s-%d: csmi_init_lib() unable to initialize. Giving up...\n", __FILE__, __LINE__);
      errno = ENOTCONN;
      return -1;
    }
    
    if ( (rc = csm_net_unix_Send(ep->_ep, msg)) == (size_t)-1 )
    {
        rc = -1;
        perror("csm_net_unix_Send");
        csmutil_logging(error, "%s-%d: csm_net_unix_send() failed\n", __FILE__, __LINE__);
    }

  return rc;
}

/**
 * \brief Recv a message from daemon
 *
 * This call blocks untis the daemon sends a valid message
 *
 * @param[in] cmd The command identifier of the message to wait for.
 *
 * @return  message struct pointer that points to library-owned memory
 *          and should not be freed by the caller. This is decided to
 *          avoid an extra copy of the data which needs to be unpacked
 *          by the caller anyway
 */
csm_net_msg_t* csmi_net_unix_Recv(csmi_cmd_t cmd) 
{
  csm_net_msg_t *msg = NULL;

  if (!initialized) {
    csmutil_logging(warning, "%s-%d: csmi_init_lib() has not been called yet\n", __FILE__, __LINE__);
    return NULL;
  }
  msg = csm_net_unix_BlockingRecv(ep->_ep, cmd);
  if ( !msg ) {
    int stored_errno = errno;
    perror("csm_netunix_Recv");
    csmutil_logging(error, "%s-%d: csm_net_unix_Recv() returned NULL\n", __FILE__, __LINE__);

    // if we received a disconnect, we need to disconnect, unless it already happened
    if( initialized  && ( stored_errno == ENOTCONN ))
      ep->_on_disconnect( NULL );
  }
  return msg;
}

/**
 * \brief register a callback function
 *
 * Allows to register a callback to be executed for messages received on the
 * secondary (callback) socket.
 *
 * @param[in]  aCallBack  the function to call (the function needs to be able
 *                        to parse a csm message as the input
 */
int csmi_register_Callback( csm_net_unix_CallBack aCallBack)
{
  if (aCallBack == NULL)
      return csm_net_unix_UnRegisterCallback(ep);
  else
      return csm_net_unix_RegisterCallback( ep, aCallBack );
} 

int csm_api_object_errcode_get(csm_api_object *csm_obj)
{
    if (csm_obj == NULL || csm_obj->hdl == NULL) 
    {
        csmutil_logging(error, "csmi_api_object not valid");
        return CSMERR_NOTDEF;
    }

    csmi_api_internal *csmi_hdl;
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    return (csmi_hdl->errcode);
}

char* csm_api_object_errmsg_get(csm_api_object *csm_obj)
{
  if (csm_obj == NULL || csm_obj->hdl == NULL) {
    csmutil_logging(error, "csmi_api_object not valid");
    return NULL;
  }

   csmi_api_internal *csmi_hdl;
   csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
   return (csmi_hdl->errmsg);
}

uint32_t csm_api_object_traceid_get(csm_api_object *csm_obj)
{
    if (csm_obj == NULL || csm_obj->hdl == NULL) {
        csmutil_logging(error, "csmi_api_object not valid");
        return -1;
    }

    csmi_api_internal *csmi_hdl;
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    return (csmi_hdl->traceid);
}

void csm_api_object_destroy(csm_api_object *csm_obj)
{
    freePrototype freeFunc;
    csmi_api_internal *csmi_hdl;

    if (csm_obj == NULL || csm_obj->hdl == NULL) {
        csmutil_logging(error, "csmi_api_object_destroy: csmi_api_object not valid");
        return;
    }
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    freeFunc = csmi_hdl->csmi_free_func;
    if (freeFunc) freeFunc(csm_obj);

    // free the space that is not csmi cmd specific and the handler itself
    csmi_api_handler_destroy(csmi_hdl);

    // finally free the csm_api_object
    free(csm_obj);
}

void csm_api_object_clear(csm_api_object *csm_obj)
{
    freePrototype freeFunc;
    csmi_api_internal *csmi_hdl;

    if (csm_obj == NULL || csm_obj->hdl == NULL) 
    {
        csmutil_logging(error, "csmi_api_object_clear: csmi_api_object not valid");
        return;
    }

    // Free the internal object.
    csmi_hdl = (csmi_api_internal *) csm_obj->hdl;
    freeFunc = csmi_hdl->csmi_free_func;
    if (freeFunc && csmi_hdl->ret_cdata != NULL) freeFunc(csm_obj);

    // Null out the returned data.
    csmi_hdl->ret_cdata = NULL;
}



