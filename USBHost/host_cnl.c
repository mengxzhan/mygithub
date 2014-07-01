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

*** Comments:      
***   This file contains the USB Host API specific functions to cancel 
***   a transfer.
***                                                               
**************************************************************************
**END*********************************************************/
#include "hostapi.h"
#include "usbprv_host.h"

#ifdef __USB_OS_MQX__
   #include "mqx_arc.h"
#endif

/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _usb_host_cancel_transfer
*  Returned Value : error or status of the transfer before cancellation
*  Comments       :
* _usb_host_cancel_transfer is a non-blocking routine that causes a transfer to 
* be terminated. 
*
*END*-----------------------------------------------------------------*/
uint_32 _usb_host_cancel_transfer
   (
      /* [IN] the USB Host state structure */
      _usb_host_handle     handle,

      /* [IN] the pipe handle */
      _usb_pipe_handle     pipe_handle,

      /* [IN] the transfer numnber for this pipe */
      uint_32              tr_number
   )
{ /* Body */

	uint_32  bRet;
#ifdef __VUSBHS__
   PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
   PIPE_TR_STRUCT_PTR         pipe_tr_ptr;
   USB_HOST_STATE_STRUCT_PTR usb_host_ptr;
#ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_cancel_transfer");
      DEBUGMSG(1, ("_usb_host_cancel_transfer\n"));
#endif

   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

   pipe_descr_ptr = (PIPE_DESCRIPTOR_STRUCT_PTR)pipe_handle;

   USB_lock();

   /* Check if valid pipe id */
   if (pipe_descr_ptr->PIPE_ID > USB_MAX_PIPES) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_cancel_transfer, invalid pipe");
         DEBUGMSG(1, ("_usb_host_cancel_transfer, invalid pipe\n"));
      #endif
      return USBERR_INVALID_PIPE_HANDLE;
   } /* Endif */

   pipe_tr_ptr = pipe_descr_ptr->tr_list_ptr;
   while (pipe_tr_ptr->TR_INDEX != tr_number)  {
      /* If unused TR found, or end of list, exit with bad status */
      if ((pipe_tr_ptr->TR_INDEX == 0) ||
         (pipe_tr_ptr->next == pipe_descr_ptr->tr_list_ptr))
      {
         USB_unlock();
         #ifdef _HOST_DEBUG_
            DEBUG_LOG_TRACE("_usb_host_cancel_transfer, invalid pipe");
            DEBUGMSG(1, ("_usb_host_cancel_transfer, invalid pipe\n"));
         #endif
         return USBERR_INVALID_PIPE_HANDLE;
      } /* EndIf */
      pipe_tr_ptr = pipe_tr_ptr->next;   
   } /* EndWhile */

   /* Get the current status */
   bRet = pipe_tr_ptr->status;

#ifdef __USB_OS_MQX__   
   /* Cancel the transfer at low-level */
   ((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
   HOST_CANCEL(handle, pipe_descr_ptr, pipe_tr_ptr);
#else
   _usb_hci_vusb20_cancel_transfer(handle, pipe_descr_ptr, pipe_tr_ptr);
#endif

   /* Indicate that the pipe is idle */
   pipe_tr_ptr->status = USB_STATUS_IDLE;

   if (pipe_tr_ptr->CALLBACK != NULL) {
      pipe_tr_ptr->CALLBACK((pointer)pipe_descr_ptr,
         pipe_tr_ptr->CALLBACK_PARAM,
         NULL,
         0,
         USBERR_SHUTDOWN);
   } /* Endif */

   USB_unlock();
#elif defined __VUSB32__
//   uint_8  bRet;
   PIPE_DESCRIPTOR_STRUCT_PTR             pipe_descr_ptr;   
   volatile USB_HOST_STATE_STRUCT _PTR_   usb_host_ptr;

   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

   pipe_descr_ptr = (PIPE_DESCRIPTOR_STRUCT_PTR)pipe_handle;

   /* Get the current status */
   bRet = pipe_descr_ptr->STATUS;

#ifdef __USB_OS_MQX__   
   /* Cancel the transfer at low-level */
   ((volatile USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
      HOST_CANCEL(handle, pipe_descr_ptr);
#else
   _usb_hci_vusb11_cancel_transfer(handle, pipe_descr_ptr);
#endif

   /* Indicate that a transfer is not pending */
   pipe_descr_ptr->PACKETPENDING = FALSE;
   
   /* Indicate that the pipe is idle */
   pipe_descr_ptr->STATUS = USB_STATUS_IDLE;
   
   /* Update the head of the queue for a particular type */
   _usb_host_update_current_head(handle, pipe_descr_ptr->PIPETYPE);
#endif

   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_cancel_transfer,SUCCESSFUL");
      DEBUGMSG(1, ("_usb_host_cancel_transfer,SUCCESSFUL\n"));
   #endif

   /* Return the status prior to the transfer cancellation */
   return bRet;

} /* Endbody */
 
/* EOF */


   

