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
***   This file contains the USB Host API specific functions to send data.
***                                                               
**************************************************************************
**END*********************************************************/
#include "hostapi.h"
#include "usbprv_host.h"

#ifdef __USB_OS_MQX__
   #include "mqx_arc.h"
#endif

#ifndef __VUSB32__
/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _usb_host_get_frame_number
*  Returned Value : frame number
*  Comments       :
* This function can be used to inquire the current frame number.
*
*END*-----------------------------------------------------------------*/
uint_32 _usb_host_get_frame_number
   (
      /* [IN] the USB Host state structure */
      _usb_host_handle  handle
   )
{ /* Body */
   USB_HOST_STATE_STRUCT_PTR usb_host_ptr;
   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_frame_number");
   #endif

#ifdef __USB_OS_MQX__
      /* Call the low-level send routine */      
   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_frame_number OS mode SUCCESSFUL");
   #endif
  
    return((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
      HOST_EHCI_GET_FRAME_NUM(handle);
#else
   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_frame_number non os SUCCESSFUL");
   #endif
   return(_usb_ehci_get_frame_number(handle));
#endif
} /* EndBody */

/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _usb_host_get_micro_frame_number
*  Returned Value : frame number
*  Comments       :
* This function can be used to inquire the current micro frame number.
*
*END*-----------------------------------------------------------------*/
uint_32 _usb_host_get_micro_frame_number
   (
      /* [IN] the USB Host state structure */
      _usb_host_handle  handle
   )
{ /* Body */
   USB_HOST_STATE_STRUCT_PTR usb_host_ptr;
   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_micro_frame_number");
   #endif

#ifdef __USB_OS_MQX__

   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_micro_frame_number OS mode SUCCESSFUL");
   #endif

      /* Call the low-level send routine */      
    return((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
      HOST_EHCI_GET_MICRO_FRAME_NUM(handle);
#else
   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_get_micro_frame_number non OS mode SUCCESSFUL");
   #endif
   return(_usb_ehci_get_micro_frame_number(handle));
#endif
} /* EndBody */
#endif

/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _usb_host_send_data
*  Returned Value : error or status of the transfer
*  Comments       :
* The Send Data routine is non-blocking routine that causes a block of data 
* to be made available for transmission to the USB host.
*
*END*-----------------------------------------------------------------*/
uint_32 _usb_host_send_data
   (
      /* [IN] the USB Host state structure */
      _usb_host_handle           handle, 

      /* [IN] the pipe handle */
      _usb_pipe_handle           pipe_handle,

      /* [IN] transfer parameters */
      TR_INIT_PARAM_STRUCT_PTR   tr_params_ptr
   )
{ /* Body */
   PIPE_DESCRIPTOR_STRUCT_PTR pipe_descr_ptr;
   USB_HOST_STATE_STRUCT_PTR  usb_host_ptr;
   uint_32  return_code;

   
#ifdef __VUSB32__
   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

   pipe_descr_ptr = (PIPE_DESCRIPTOR_STRUCT_PTR)pipe_handle;

   /* Check if the pipe id is valid */
   if (pipe_descr_ptr->PIPE_ID > USB_MAX_PIPES) {
      #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_send_data inalid pipe handle");
      #endif
      return USBERR_INVALID_PIPE_HANDLE;
   } /* Endif */

   USB_lock();

   /* Check if a previously queued transfer is still in progress */
   if (pipe_descr_ptr->STATUS != USB_STATUS_IDLE) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_send_data transfer in progress");
      #endif
      return USB_STATUS_TRANSFER_IN_PROGRESS;
   } else {
      USB_unlock();
      /* Initialize the pipe descriptor with the new send request */
      pipe_descr_ptr->TR = *tr_params_ptr;
      pipe_descr_ptr->SOFAR = 0;
      pipe_descr_ptr->TX1_PTR = tr_params_ptr->TX_BUFFER;

      if ((pipe_descr_ptr->PIPETYPE == USB_ISOCHRONOUS_PIPE) && 
         (pipe_descr_ptr->TODO1 > pipe_descr_ptr->MAX_PACKET_SIZE)) 
      {
         /* For isochronous pipes, we transfer a mzximum of MAX_PACKET_SIZE 
         ** packet 
         */
         pipe_descr_ptr->TODO1 = pipe_descr_ptr->MAX_PACKET_SIZE;
      } else {
         /* For all others max of 64KB */
         pipe_descr_ptr->TODO1 = tr_params_ptr->TX_LENGTH;
      } /* Endif */

      #ifdef _DATA_CACHE_
      /*Must Flush and Invalidate the buffer before sending
      /receiving the data in it */
      USB_dcache_flush_mlines(
            (void *)pipe_descr_ptr->TX1_PTR,
            tr_params_ptr->TX_LENGTH
      );
      #endif
      

      USB_lock();
      
      /* return successful transfer initiation status */
      pipe_descr_ptr->STATUS = USB_STATUS_TRANSFER_QUEUED;

#ifdef __USB_OS_MQX__

      /* Call the low-level send routine */      
      ((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
         HOST_SEND(handle, pipe_descr_ptr);
#else
      _usb_hci_vusb11_send_data(handle, pipe_descr_ptr);
#endif

      /* Indicate that a packet is pending */      
      pipe_descr_ptr->PACKETPENDING = TRUE;

      /*full speed driver assumes that transfer is queued. It should be
      changed in future changes */
      
      return_code =  USB_STATUS_TRANSFER_QUEUED;
      
      USB_unlock();
   } /* Endif */

#else
   PIPE_TR_STRUCT_PTR         pipe_tr_ptr;
#ifdef _HOST_DEBUG_
   DEBUG_LOG_TRACE("_usb_host_send_data");
#endif
   USB_lock();
   return_code = _usb_host_set_up_TR(pipe_handle, tr_params_ptr, &pipe_tr_ptr);
   if (return_code != USB_STATUS_TRANSFER_QUEUED) {
      USB_unlock();
      #ifdef _HOST_DEBUG_
         DEBUG_LOG_TRACE("_usb_host_send_data failed to queue transfer");
      #endif

      return return_code;
   } /* Endif */
      
   usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;
   pipe_descr_ptr = (PIPE_DESCRIPTOR_STRUCT_PTR)pipe_handle;

   #ifdef _DATA_CACHE_
   /*Must Flush and Invalidate the buffer before sending
   /receiving the data in it */
   USB_dcache_flush_mlines(
         (void *)pipe_tr_ptr->TX_BUFFER,
         pipe_tr_ptr->TX_LENGTH
   );
   #endif
      

   /* We have obtained the current TR on the Pipe's TR list 
   ** from _usb_host_set_up_TR
   */
#ifdef __USB_OS_MQX__
      /* Call the low-level send routine */      
   return_code = ((USB_CALLBACK_FUNCTIONS_STRUCT_PTR)usb_host_ptr->CALLBACK_STRUCT_PTR)->\
                  HOST_SEND(handle, pipe_descr_ptr, pipe_tr_ptr);
#else
   return_code = _usb_hci_vusb20_send_data(handle, pipe_descr_ptr, pipe_tr_ptr);
#endif

   USB_unlock();
   
#endif

   #ifdef _HOST_DEBUG_
      DEBUG_LOG_TRACE("_usb_host_send_data SUCCESSFUL");
   #endif

   if(return_code == USB_OK)
   {
      return USB_STATUS_TRANSFER_QUEUED;
   }
   else
   {
      return  return_code;
   }

} /* Endbody */

/* EOF */


   

