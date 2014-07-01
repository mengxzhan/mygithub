/**HEADER*********************************************************
-----------------------------------------------------------------------------
 CONFIDENTIAL AND PROPRIETARY INFORMATION OF TRANSDIMENSION, INC. AND/OR ITS
 WHOLLY OWNED SUBSIDIARY SOFTCONNEX TECHNOLOGIES, INC. THIS NOTICE IS NOT TO
 BE DELETED, MODIFIED, MOVED OR CHANGED IN ANY WAY.

 (TransDimension, Inc. and SoftConnex Technologies, Inc. are referred to
 collectively below as TransDimension, Inc.) 
 Copyright (c) 1999 - 2004 by TransDimension, Inc. 

 This software is protected by copyright laws and international copyright 
 treaties, as well as other intellectual property laws and treaties.  This
 software is a CONFIDENTIAL, unpublished work of authorship, and with portions 
 constituting TRADE SECRETS of TransDimension, Inc..  Any unauthorized use, 
 disclosure, and/or reproduction of this software, or any part of this software;
 or distribution of this software in any form or by any means; or storage of 
 this software in any database or retrieval system, without the express written
 consent of, and license from, TransDimension, Inc. is strictly prohibited.  
 This software is protected under the copyright and/or trade secret laws in 
 other countries in addition to USA.  All Rights Reserved.  Failure to abide by
 the use, disclosure and/or reproduction restrictions may result in civil and
 or criminal penalties, and will be prosecuted to the maximum extent of the law.
 -------------------------------------------------------------------------------

*** Description:      
***   This file contains standard device requests (USB Chapter 9)
***
**************************************************************************
**END*********************************************************/
#include "hostapi.h"
#include "usbprv.h"
#include "host_ch9.h"
#include "host_dev_list.h"
#include "host_common.h"

#include "BU_interrupt.h"

#ifdef __USB_OS_MQX__
   #include "mqx_arc.h"
#else
   #include "usbprv_host.h"
#endif

#ifdef _DBUG_
extern uint_32 trace_index;
#endif

/* Prototype, copy to stack frames for each use instance */

static const USB_SETUP req_prototype =
{
   0,          /* request type */
   0,          /* request */
   {0,0},      /* WVALUE  */
   {0,0},      /* WINDEX  */
   {0,0}       /* WLENGTH  */
};

/* Unpublished functions, not intended for application use */
extern void  usb_hostdev_attach_detach(DEV_INSTANCE_PTR, uint_32);


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_cntrl_transaction_done
* Returned Value : none
* Comments       :
*     Callback function to process transaction-done events
*     State machine for enumeration/subsequent transactions
*     Between states, the 8-byte buffer in device_instance
*        is used to save the first part of the configuration.
*     Pointer desc in various flavors (uchar_ptr, cfig_ptr)
*        is used to parse this buffer in various ways.
*
*END*--------------------------------------------------------------------*/
static void  usb_host_cntrl_transaction_done
   (
      /* [IN] the pipe handle */
      _usb_pipe_handle  handle,

      /* [IN] The user parameter */
      pointer           user_parm,

      /* [IN] the buffer */
      uchar_ptr         buffer,

      /* [IN] The length of the transfer */
      uint_32           length,

      /* [IN] status of the transfer */
      uint_32           status
   )
{ /* Body */
   USB_HOST_STATE_STRUCT_PTR  usb_host_ptr;
   PIPE_DESCRIPTOR_STRUCT_PTR pipe_ptr =
      (PIPE_DESCRIPTOR_STRUCT_PTR)handle;
   DEV_INSTANCE_PTR           dev_inst_ptr =
      (DEV_INSTANCE_PTR)pipe_ptr->DEV_INSTANCE;
   DESCRIPTOR_UNION           desc;
   // clean warning pointer                    temp = NULL;
   uint_32                    config_size;

   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("usb_host_cntrl_transaction_done");
      DEBUGMSG(USB_DBG_LEVEL, ("usb_host_cntrl_transaction_done\n"));
   #endif
   DEBUGMSG(USB_DBG_LEVEL, ("usb_host_cntrl_transaction_done, %d\n", dev_inst_ptr->state));
   usb_host_ptr = dev_inst_ptr->host;

   /*----------------------------------------------------**
   ** Enumeration state machine -- cases are named after **
   ** the just-completed transactions.                   **
   **----------------------------------------------------*/

   switch (dev_inst_ptr->state) {
      case DEVSTATE_INITIAL:      /* initial device state */
         break;
      case DEVSTATE_DEVDESC8:     /* device descriptor [0..7]*/
         /* We must have received the first 8 bytes in
         ** dev_inst_ptr->dev_descriptor which contains the
         ** max packet size for this control endpoint
         */
         pipe_ptr->MAX_PACKET_SIZE =
            dev_inst_ptr->dev_descriptor.bMaxPacketSize;
         dev_inst_ptr->state = DEVSTATE_ADDR_SET;
         /* Now set the address that we assigned when we initialized
         ** the device instance struct for this device
         */
         _usb_host_ch9_set_address((pointer)dev_inst_ptr);
         break;
      case DEVSTATE_ADDR_SET:     /* address set */
#ifndef __VUSB32__
#ifdef __USB_OS_MQX__
         /* Call the low-level routine to free the controller specific resources */
         ((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)
            usb_host_ptr->CALLBACK_STRUCT_PTR)->\
            HOST_FREE_CONTROLLER_RESOURCE((pointer)usb_host_ptr, pipe_ptr);
#else
         /* Call the low-level routine to free the controller specific resources */
         _usb_ehci_free_resources((pointer)usb_host_ptr, pipe_ptr);
#endif
#endif
         pipe_ptr->DEVICE_ADDRESS = dev_inst_ptr->address;
         dev_inst_ptr->state = DEVSTATE_DEV_DESC;

         /* Now get the full descriptor */
         _usb_host_ch9_get_descriptor((pointer)dev_inst_ptr,
            DESCTYPE_DEVICE<<8,
            0,
            sizeof(DEVICE_DESCRIPTOR),
            (uchar_ptr)&dev_inst_ptr->dev_descriptor);
         break;
      case DEVSTATE_DEV_DESC:     /* full device descriptor received */
         /* Now lets get the first 9 bytes of the configuration
         ** descriptor
         */
         desc.pntr = &dev_inst_ptr->buffer;
         dev_inst_ptr->state = DEVSTATE_GET_CFG9;
         _usb_host_ch9_get_descriptor((pointer)dev_inst_ptr,
            DESCTYPE_CONFIG<<8,
            0,
            sizeof(dev_inst_ptr->buffer),
            desc.bufr);
         break;
      case DEVSTATE_GET_CFG9:     /* Read 9 bytes of config descriptor */
         dev_inst_ptr->state = DEVSTATE_SET_CFG;
         /* Now select the configuration as specified in the
         ** descriptor
         */
         desc.cfig = (CONFIGURATION_DESCRIPTOR_PTR)dev_inst_ptr->buffer;
         config_size = utoh16(desc.cfig->wTotalLength);

         desc.pntr = &dev_inst_ptr->buffer;
         if (USB_OK != usb_dev_list_get_memory(dev_inst_ptr,
            config_size,
            USB_MEMTYPE_CONFIG,
            (pointer)&desc))
         {
            #ifdef _HOST_DEBUG_
               DEBUG_LOG_TRACE("usb_host_cntrl_transaction_done");
            #endif
            return;
         }
         desc.word += MEM_HEADER_LEN; /* step past header */
#ifdef __USB_OTG__
         dev_inst_ptr->state = DEVSTATE_CHK_OTG;
#else
         dev_inst_ptr->state = DEVSTATE_SET_CFG;
#endif
         /* We can only read one config descriptor at a time */
         _usb_host_ch9_get_descriptor((pointer)dev_inst_ptr,
            DESCTYPE_CONFIG<<8,
            0,
            config_size,
            desc.bufr);
         break;

      case DEVSTATE_CHK_OTG:
         /* Point to the memory owned by this device */
         desc.pntr = &dev_inst_ptr->memlist->payload;

         /* We will always start with config desc so update the search pointer */
         config_size = utoh16(desc.cfig->wTotalLength);
         config_size -= desc.cfig->bLength;
         desc.word += desc.cfig->bLength;

         while (config_size) {
            if (desc.otg->bDescriptorType == DESCTYPE_OTG) {
               /* Found the OTG descriptor */
               break;
            } /* End if */
            config_size -= desc.intf->bLength;
            desc.word += desc.intf->bLength;
         } /* EndWhile */

         /* Check for an OTG descriptor */
         dev_inst_ptr->state = DEVSTATE_SET_CFG;

         if (config_size && desc.otg->bDescriptorType == DESCTYPE_OTG &&
            (desc.otg->bmAttributes & OTG_HNP_SUPPORT))
         {
            _usb_host_ch9_set_feature(dev_inst_ptr, 0, 0, 4);
            break;
         } /* End if */

         /* Fall through */

      case DEVSTATE_SET_CFG:     /* config descriptor [0..8] */

         /* Point to the memory owned by this device */
         desc.pntr = &dev_inst_ptr->memlist->payload;
         dev_inst_ptr->state = DEVSTATE_CFG_READ;
         _usb_host_ch9_set_configuration(dev_inst_ptr,
            desc.cfig->bConfigurationValue);
         break;

      case DEVSTATE_CFG_READ:     /* full config desc. read in */
         dev_inst_ptr->state = DEVSTATE_APP_CALL;

         /* Scan the configuration descriptor to find out the total
         ** number of interfaces available. This will be the upper
         ** bound for searching through the interface descriptors'
         ** array
         */
         dev_inst_ptr->num_of_interfaces = 0;

         /* Point to the memory owned by this device */
         desc.pntr = &dev_inst_ptr->memlist->payload;
         /* We will always start with config desc so update the search pointer */
         config_size = utoh16(desc.cfig->wTotalLength);
         config_size -= desc.cfig->bLength;
         desc.word += desc.cfig->bLength;
         while (config_size) {
            if (desc.intf->bDescriptorType == DESCTYPE_INTERFACE) {
               /* Found an interface */
               dev_inst_ptr->num_of_interfaces++;
            } /* Endif */
            config_size -= desc.intf->bLength;
            desc.word += desc.intf->bLength;
         } /* EndWhile */

         /* Don't select an interface here. The device driver will
         ** select the interface
         */

      case DEVSTATE_APP_CALL:     /* full config desc. read in */
         dev_inst_ptr->state = DEVSTATE_SET_INTF;
         if (dev_inst_ptr->new_config != 0) {
            /* We have just read a new configuration descriptor */
            dev_inst_ptr->new_config = 0;
            usb_hostdev_attach_detach(dev_inst_ptr, USB_CONFIG_EVENT);
         } else {
            usb_hostdev_attach_detach(dev_inst_ptr, USB_ATTACH_EVENT);
         } /* EndIf */
         break;

      case DEVSTATE_SET_INTF:    /* Select interface done */
    	  DEBUGMSG(USB_DBG_LEVEL, ("intf \r\n"));
         dev_inst_ptr->state = DEVSTATE_ENUM_OK;
         usb_hostdev_attach_detach(dev_inst_ptr, USB_INTF_EVENT);
         break;
      default:
         dev_inst_ptr->state = DEVSTATE_ENUM_OK;
         case DEVSTATE_ENUM_OK:   /* enumeration complete */
            if ((dev_inst_ptr->control_callback != NULL))
               dev_inst_ptr->control_callback
                  (handle,
                  user_parm,
                  buffer,
                  length,
                  status);
            break;
      } /* EndSwitch */
      #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("usb_host_cntrl_transaction_done SUCCESSFUL");
      #endif

} /* EndBody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_register_ch9_callback
* Returned Value : USB_OK, or error status
* Comments       :
*     This function registers a callback function that will be called
*  to notify the user of a ch9 command completion. This should be used
*  only after enumeration is completed
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_register_ch9_callback
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* Callback upon completion */
      tr_callback                   callback,

      /* User provided callback param */
      pointer                       callback_param
   )
{ /* Body */
   DEV_INSTANCE_PTR           dev_inst_ptr;
   USB_STATUS                 error;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_register_ch9_callback");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_register_ch9_callback\n"));
   #endif
   /* Verify that device handle is valid */
   USB_lock();
   error = usb_hostdev_validate(dev_handle);

   if (error != USB_OK) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("_usb_host_register_ch9_callback, device not found");
            DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_register_ch9_callback, device not found\n"));
      #endif
      return USBERR_DEVICE_NOT_FOUND;
   } /* Endif */

   dev_inst_ptr = (DEV_INSTANCE_PTR)dev_handle;

   /* This will be called if the device is already enumerated */
   dev_inst_ptr->control_callback = callback;
   dev_inst_ptr->control_callback_param = callback_param;

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_register_ch9_callback, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_register_ch9_callback, SUCCESSFUL\n"));
   #endif
   return USB_OK;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_ch9_dev_req
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device requests in Chapter 9.
*        See Table 9-3 p. 250 of USB 2.0 specification.
*     This code does minimal error checking, on the assumption
*        that it is called only from wrappers in this file.
*     It is presumed that this function is called with USB interrupts disabled
*
*END*--------------------------------------------------------------------*/
static USB_STATUS  usb_host_ch9_dev_req
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* Device Request to send */
      USB_SETUP_PTR                 devreq_ptr,
         
      /* buffer to send/receive */
      uchar_ptr                     buff_ptr
   )
{ /* Body */
  
   DEV_INSTANCE_PTR           dev_ptr;
   _usb_pipe_handle           pipe_handle;
   TR_INIT_PARAM_STRUCT       tr;
   USB_STATUS                 error;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("usb_host_ch9_dev_req");
         DEBUGMSG(USB_DBG_LEVEL, ("usb_host_ch9_dev_req\n"));
   #endif

	//DEBUGMSG(TTC_DBG_LEVEL,("usb_host_ch9_dev_req\r\n"));

   /* Verify that device handle is valid */
   error = usb_hostdev_validate(dev_handle);

   if (error != USB_OK)
   {

      #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("usb_host_ch9_dev_req device not found");
            DEBUGMSG(USB_DBG_LEVEL, ("usb_host_ch9_dev_req device not found\n"));
      #endif
      return USBERR_DEVICE_NOT_FOUND;
   } /* Endif */

   dev_ptr = (DEV_INSTANCE_PTR)dev_handle;
   pipe_handle = dev_ptr->control_pipe;

   /* Initialize the TR with TR index and default control callback
   ** function if there is one registered
   */
   usb_hostdev_tr_init(&tr, dev_ptr->control_callback,
      dev_ptr->control_callback_param);

	tr.TX_LENGTH = 0;
   /* Set buffer length if required */
   switch (devreq_ptr->BREQUEST) {
      case REQ_SET_DESCRIPTOR:
         tr.TX_BUFFER = buff_ptr;
         tr.TX_LENGTH = utoh16(devreq_ptr->WLENGTH);
         break;
      case REQ_GET_CONFIGURATION:
      case REQ_GET_DESCRIPTOR:
      case REQ_GET_INTERFACE:
      case REQ_GET_STATUS:
      case REQ_SYNCH_FRAME:
         tr.RX_BUFFER = buff_ptr;
         tr.RX_LENGTH = utoh16(devreq_ptr->WLENGTH);
         break;
   } /* EndSwitch */

   tr.DEV_REQ_PTR = (uchar_ptr)(devreq_ptr);

   if ((dev_ptr->state < DEVSTATE_ENUM_OK) ||
      (tr.CALLBACK == NULL))
   {
//	  DEBUGMSG(USB_DBG_LEVEL, ("usb_Host_ch9_dev_req register callback function\n"));
      tr.CALLBACK = usb_host_cntrl_transaction_done;
      tr.CALLBACK_PARAM = NULL;
   } /* End if */

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("usb_host_ch9_dev_req SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("usb_host_ch9_dev_req SUCCESSFUL\n"));
   #endif
   return (_usb_host_send_setup(dev_ptr->host, pipe_handle, &tr));

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_clear_feature
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_clear_feature
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* request type device/interface/endpoint */
      uint_8                        req_type,

      /* device = 0, or interface/endpoint */
      uint_8                        intf_endpt,

      /* feature selection */
      uint_16                       feature
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_clear_feature");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_clear_feature\n"));
   #endif

   USB_lock();

   switch (req_type) {
      case REQ_TYPE_DEVICE:
         break;
      case REQ_TYPE_INTERFACE:
      case REQ_TYPE_ENDPOINT:
         htou16(request.WINDEX, intf_endpt);
         break;
      default:
         USB_unlock();
         #ifdef _HOST_DEBUG_
               DEBUG_LOG_TRACE("_usb_host_ch9_clear_feature, invalid request");
               DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_clear_feature, invalid request\n"));
         #endif
         return USBERR_INVALID_BMREQ_TYPE;
   } /* EndSwitch */

   request.BMREQUESTTYPE = req_type | REQ_TYPE_OUT;
   request.BREQUEST = REQ_CLEAR_FEATURE;
   htou16(request.WVALUE, feature);

   error = usb_host_ch9_dev_req(dev_handle, &request, NULL);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_clear_feature, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_clear_feature, SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_get_configuration
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_get_configuration
   (
      /* [IN] usb device */
      _usb_device_instance_handle   dev_handle,

      /* [OUT] configuration value */
      uchar_ptr                     buffer
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_configuration");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_configuration\n"));
   #endif

//	DEBUGMSG(TTC_DBG_LEVEL,("_usb_host_ch9_get_configuration\r\n"));

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_DEVICE | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_CONFIGURATION;
   htou16(request.WLENGTH, 1);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_configuration, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_configuration, SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_get_descriptor
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_get_descriptor
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,
      /* descriptor type & index */
      uint_16                       type_index,
      /* Language ID or 0 */
      uint_16                       lang_id,
      /* buffer length */
      uint_16                       buflen,
      /* descriptor buffer */
      uchar_ptr                     buffer
   )
{ /* Body */

   USB_SETUP  request;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_descriptor");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_descriptor\n"));
   #endif

	DEBUGMSG(USB_DBG_LEVEL,("_usb_host_ch9_get_descriptor\r\n"));

   USB_lock();
   request.BMREQUESTTYPE = REQ_TYPE_DEVICE | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_DESCRIPTOR;
   htou16(request.WVALUE, type_index);
   htou16(request.WINDEX, lang_id);
   htou16(request.WLENGTH, buflen);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_descriptor, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_descriptor, SUCCESSFUL\n"));
   #endif

   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_get_interface
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_get_interface
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* interface index */
      uint_8            interface,

      /* alternate setting buffer */
      uchar_ptr         buffer
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_interface");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_interface\n"));
   #endif

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_INTERFACE | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_INTERFACE;
   htou16(request.WINDEX, interface);
   htou16(request.WLENGTH, 1);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_interface, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_interface, SUCCESSFUL\n"));
   #endif

   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_get_status
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_get_status
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* request type device/interface/endpoint */
      uint_8            req_type,

      /* device = 0, or interface/endpoint */
      uint_8            intf_endpt,

      /* returned status */
      uchar_ptr         buffer
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_status");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_status\n"));
   #endif

   USB_lock();

   switch (req_type) {
      case REQ_TYPE_DEVICE:
      case REQ_TYPE_INTERFACE:
      case REQ_TYPE_ENDPOINT:
         break;
      default:
         USB_unlock();
         #ifdef _HOST_DEBUG_
               DEBUG_LOG_TRACE("_usb_host_ch9_get_status, invalid parameter");
               DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_status, invalid parameter\n"));
         #endif
         return USBERR_INVALID_BMREQ_TYPE;
   } /* EndSwitch */

   request.BMREQUESTTYPE = req_type | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_STATUS;
   htou16(request.WINDEX, intf_endpt);
   htou16(request.WLENGTH, 2);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);
   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_get_status, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_get_status, SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */

 
/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_set_address
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_set_address
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle
   )
{ /* Body */

   DEV_INSTANCE_PTR  dev_ptr = (DEV_INSTANCE_PTR)dev_handle;
   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_address");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_address\n"));
   #endif
   DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_address\n"));
   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_DEVICE | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_ADDRESS;
   htou16(request.WVALUE, dev_ptr->address);

   error = usb_host_ch9_dev_req(dev_handle, &request, NULL);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_address,SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_address,SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_set_configuration
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_set_configuration
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* configuration value */
      uint_16                       config
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_configuration");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_configuration\n"));
   #endif

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_DEVICE | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_CONFIGURATION;
   htou16(request.WVALUE, config);

   error = usb_host_ch9_dev_req(dev_handle, &request, NULL);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_configuration SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_configuration SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_set_descriptor
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_set_descriptor
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* descriptor type & index */
      uint_16           type_index,

      /* Language ID or 0 */
      uint_16           lang_id,

      /* buffer length */
      uint_16           buflen,

      /* descriptor buffer */
      uchar_ptr         buffer
   )
{ /* Body */

   USB_SETUP  request;
   USB_STATUS error = USB_OK;
   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_descriptor");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_descriptor\n"));
   #endif

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_DEVICE | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_DESCRIPTOR;
   htou16(request.WVALUE, type_index);
   htou16(request.WINDEX, lang_id);
   htou16(request.WLENGTH, buflen);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_descriptor SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_descriptor SUCCESSFUL\n"));
   #endif

   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_set_feature
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_set_feature
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* request type device/interface/endpoint */
      uint_8            req_type,

      /* device = 0, or interface/endpoint */
      uint_8            intf_endpt,

      /* feature selection */
      uint_16           feature
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_feature");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_feature\n"));
   #endif
   DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_feature\n"));
   USB_lock();
   switch (req_type) {
      case REQ_TYPE_DEVICE:
         break;
      case REQ_TYPE_INTERFACE:
      case REQ_TYPE_ENDPOINT:
         htou16(request.WINDEX, intf_endpt);
         break;
      default:
         USB_unlock();
         #ifdef _HOST_DEBUG_
               DEBUG_LOG_TRACE("_usb_host_ch9_set_feature, invalid param");
               DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_feature, invalid param\n"));
         #endif
         return USBERR_INVALID_BMREQ_TYPE;
   } /* EndSwitch */

   request.BMREQUESTTYPE = req_type | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_FEATURE;
   htou16(request.WVALUE, feature);

   error = usb_host_ch9_dev_req(dev_handle, &request, NULL);

   USB_unlock();
   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_feature, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_feature, SUCCESSFUL\n"));
   #endif
   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_set_interface
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_set_interface
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* alternate setting */
      uint_8            alternate,

      /* interface */
      uint_8            intf
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_interface");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_interface\n"));
   #endif

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_INTERFACE | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_INTERFACE;
   htou16(request.WVALUE, alternate);
   htou16(request.WINDEX, intf);

   error = usb_host_ch9_dev_req(dev_handle, &request, NULL);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_set_interface, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_set_interface, SUCCESSFUL\n"));
   #endif

   return error;

} /* EndBody */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_host_ch9_synch_frame
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process standard device request in Chapter 9.
*     See Table 9-3 p. 250 of USB 2.0 specification.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_host_ch9_synch_frame
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* interface index */
      uint_8            interface,

      /* configuration buffer */
      uchar_ptr         buffer
   )
{ /* Body */

   USB_SETUP  request = req_prototype;
   USB_STATUS error = USB_OK;

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_synch_frame");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_synch_frame\n"));
   #endif

   USB_lock();

   request.BMREQUESTTYPE = REQ_TYPE_ENDPOINT | REQ_TYPE_IN;
   request.BREQUEST = REQ_SYNCH_FRAME;
   htou16(request.WINDEX, interface);
   htou16(request.WLENGTH, 2);

   error = usb_host_ch9_dev_req(dev_handle, &request, buffer);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_ch9_synch_frame, SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_host_ch9_synch_frame, SUCCESSFUL\n"));
   #endif
   
   return error;

} /* EndBody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : _usb_hostdev_cntrl_request
* Returned Value : USB_OK, or error status
* Comments       :
*     Function to process class- or vendor-specific control pipe device
*     requests.
*
*END*--------------------------------------------------------------------*/
USB_STATUS  _usb_hostdev_cntrl_request
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,

      /* Device Request to send */
      USB_SETUP_PTR                 devreq,

      /* buffer to send/receive */
      uchar_ptr                     buff_ptr,

      /* callback upon completion */
      tr_callback                   callback,

      /* [IN] the parameter to pass back to the callback function */
      pointer                       callback_param
   )
{ /* Body */

   DEV_INSTANCE_PTR           dev_ptr;
   _usb_pipe_handle           pipe_handle;
   TR_INIT_PARAM_STRUCT       tr;
   USB_STATUS                 error = USB_OK;


   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_hostdev_cntrl_request");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_hostdev_cntrl_request\n"));
   #endif
 
//	DEBUGMSG(TTC_DBG_LEVEL,("_usb_hostdev_cntrl_request\r\n"));
  
   /* Verify that device handle is valid */
   USB_lock();
   error = usb_hostdev_validate(dev_handle);

   if (error != USB_OK) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("_usb_hostdev_cntrl_request, invalid device handle");
            DEBUGMSG(USB_DBG_LEVEL, ("_usb_hostdev_cntrl_request, invalid device handle\n"));
      #endif
      return USBERR_DEVICE_NOT_FOUND;
   } /* Endif */
      

   dev_ptr = (DEV_INSTANCE_PTR)dev_handle;

   if (dev_ptr->state < DEVSTATE_ENUM_OK) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("_usb_hostdev_cntrl_request, no device found");
      #endif
      return USBERR_DEVICE_NOT_FOUND;
   } /* Endif */

   pipe_handle = dev_ptr->control_pipe;

   usb_hostdev_tr_init(&tr, callback, callback_param);

   /* Set TR buffer length as required */
   if ((REQ_TYPE_IN & devreq->BMREQUESTTYPE) != 0) {
      tr.RX_BUFFER = buff_ptr;
      tr.RX_LENGTH = utoh16(devreq->WLENGTH);
   } else {
      tr.TX_BUFFER = buff_ptr;
      tr.TX_LENGTH = utoh16(devreq->WLENGTH);
   } /* EndIf */

   tr.DEV_REQ_PTR = (uchar_ptr)devreq;

   error = _usb_host_send_setup(dev_ptr->host, pipe_handle, &tr);

   USB_unlock();

   #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_hostdev_cntrl_request,SUCCESSFUL");
         DEBUGMSG(USB_DBG_LEVEL, ("_usb_hostdev_cntrl_request,SUCCESSFUL\n"));
   #endif
   
   return error;
} /* EndBody */


/* EOF */
