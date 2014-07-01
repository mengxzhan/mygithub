/*******************************************************************************
** File          : $HeadURL$ 
** Author        : $Author: kgupta $
** Project       : HSCTRL 
** Instances     : 
** Creation date : 
********************************************************************************
********************************************************************************
** ChipIdea Microelectronica - IPCS
** TECMAIA, Rua Eng. Frederico Ulrich, n 2650
** 4470-920 MOREIRA MAIA
** Portugal
** Tel: +351 229471010
** Fax: +351 229471011
** e_mail: chipidea.com
********************************************************************************
** ISO 9001:2000 - Certified Company
** (C) 2005 Copyright Chipidea(R)
** Chipidea(R) - Microelectronica, S.A. reserves the right to make changes to
** the information contained herein without notice. No liability shall be
** incurred as a result of its use or application.
********************************************************************************
** Modification history:
** $Date: 2013/03/12 23:30:18 $
** $Revision: 1.2 $
*******************************************************************************
*** Description:
***   This file is an example of device drivers for a Mouse device.
*** It has been tested in a Dell and logitech USB 3 button wheel Mouse. The program
*** queues transfers on Interrupt USB pipe and waits till the data comes
*** back. It prints the data and queues another transfer on the same pipe.
*** See the code for details.
***
**************************************************************************
**END*********************************************************/

#include "usbh_types.h"


/* skip the inclusion in dependency stage */
#ifndef __NO_SETJMP
   #include "stdio.h"
#endif
#include "stdlib.h"

#include "BU_interrupt.h"
#include "hid_mouse.h"
#include "usb_hid.h"
#include "Usbprv_host.h"
#include "BU_SWconf.h"
/************************************************************************************
**
** Globals
************************************************************************************/

volatile boolean Interrupt_received_mouse = FALSE; /* This boolean controls queuing of transfers */

volatile DEVICE_STRUCT  hid_device_mouse = { 0 };  /* structure defined by this driver */

volatile uint_32        hid_mouse_conf_phase = 0;  /*simple integer to track phases    */

_usb_host_handle        host_handle_mouse;         /* global handle for calling host   */

volatile boolean bHidCallBack = FALSE;
/************************************************************************************
Table of driver capabilities this application wants to use. See Host API document for
details on How to define a driver info table. This table defines that this driver
supports a HID class, boot subclass and mouse protocol.
************************************************************************************/

static  USB_HOST_DRIVER_INFO DriverInfoTable[] =
{
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_HID,                /* Class code                       */
      USB_SUBCLASS_HID_BOOT,        /* Sub-Class code                   */
      USB_PROTOCOL_HID_MOUSE,       /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_hid_mouse_event      /* Application call back function   */
   },
   {
      {0x00,0x00},                  /* All-zero entry terminates        */
      {0x00,0x00},                  /* driver info list.                */
      0,
      0,
      0,
      0,
      NULL
   }
};


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : main (Main_Task if using MQX)
* Returned Value : none
* Comments       :
*     Execution starts here
*
*END*--------------------------------------------------------------------*/


void HID_MOUSE_TEST (void)
{
   USB_STATUS              status = USB_OK;
   _usb_pipe_handle        pipe;
   TR_INIT_PARAM_STRUCT    tr,tr1,tr2;
   HID_COMMAND_PTR         hid_com;
   uchar_ptr               buffer;


   /*******************************************************
   Allocate memory for being able to send commands to HID
   class driver. Note that USB_memalloc() allocates aligned
   buffers on cache line boundry. This is required if
   program is running with data cache on system.
   *******************************************************/

   hid_com = (HID_COMMAND_PTR) USB_Uncached_memalloc (sizeof(HID_COMMAND));

   _disable_interrupts();
   status = _usb_host_init
       (HOST_CONTROLLER_NUMBER,   /* Use value in header file */
        MAX_FRAME_SIZE,            /* Frame size per USB spec  */
        &host_handle_mouse);             /* Returned pointer */

   if (status != USB_OK) {
        DEBUGMSG(USB_DBG_LEVEL,("\nUSB Host Initialization failed. STATUS: %x\n", status));
   } /* End if */

   /*
    ** since we are going to act as the host driver, register the driver
    ** information for wanted class/subclass/protocols
   */
   status = _usb_host_driver_info_register
        (host_handle_mouse,
        DriverInfoTable);
   if (status != USB_OK) {
    	DEBUGMSG(USB_DBG_LEVEL,("\nDriver Registration failed. STATUS: %x \n", status));
   }

   /*******************************************************
    Allocate memory for receiving data from USB mouse
   *******************************************************/
   buffer = (uchar_ptr)USB_Uncached_memalloc(HID_BUFFER_SIZE);
   if (!buffer) {
      DEBUGMSG(USB_DBG_LEVEL,("\nError allocating buffer!\n"));
      exit(1);
   } /* End if */

//   INT_Enable( INT_USB, 15 );
   IRQ_INT_Enable( INT_USB_OTG, 15 );
   /*
   ** Infinite loop, waiting for events requiring action
   */
   for ( ; ; ) {
      switch ( hid_device_mouse.DEV_STATE ) {
         case USB_DEVICE_IDLE:
            break;
         case USB_DEVICE_ATTACHED:
        	DEBUGMSG(USB_DBG_LEVEL,("\nHid device attached\n"));
            hid_device_mouse.DEV_STATE = USB_DEVICE_SET_INTERFACE_STARTED;
            status = _usb_hostdev_select_interface(hid_device_mouse.DEV_HANDLE,
               hid_device_mouse.INTF_HANDLE, (pointer)&hid_device_mouse.CLASS_INTF);
              if (status != USB_OK) {
               DEBUGMSG(USB_DBG_LEVEL,("\nError in _usb_hostdev_select_interface: %x", status));
               exit(1);
            } /* End if */
            break;
         case USB_DEVICE_SET_INTERFACE_STARTED:
            break;
         case USB_DEVICE_INTERFACED:
            if (!hid_mouse_conf_phase)
            {
               hid_com->CLASS_PTR = (CLASS_CALL_STRUCT_PTR)&hid_device_mouse.CLASS_INTF;
               hid_com->CALLBACK_PARAM = 0;
               hid_com->CALLBACK_FN = usb_host_hid_mouse_user_callback;
               hid_mouse_conf_phase = 2;
               bHidCallBack = FALSE;
               usb_class_hid_set_idle(hid_com,0,0);
               while(!bHidCallBack){;}

            } else if (hid_mouse_conf_phase == 1)
            {
               /* Do nothing */
            }
            else

            {
               pipe = _usb_hostdev_find_pipe_handle(hid_device_mouse.DEV_HANDLE,
               hid_device_mouse.INTF_HANDLE, USB_INTERRUPT_PIPE, USB_RECV);
               if (pipe)
               {
            	   DEBUGMSG(USB_DBG_LEVEL,("hid num: %d \n",((PIPE_DESCRIPTOR_STRUCT_PTR)pipe)->ENDPOINT_NUMBER));
                   usb_hostdev_tr_init(&tr, usb_host_hid_mouse_recv_callback, NULL);
                   tr.RX_BUFFER = buffer;
                   tr.RX_LENGTH = HID_BUFFER_SIZE;
                   status = _usb_host_recv_data(host_handle_mouse, pipe, &tr);
                   if (status != USB_STATUS_TRANSFER_QUEUED) {
                   DEBUGMSG(USB_DBG_LEVEL,("\nError in _usb_host_recv_data: %x", status));
                   return;
                }

                /******************************************************************
                 Wait in a loop till we get the data from mouse. Note that this
                 is not necessary to loop and an application could proceed and
                 do something else. It will notified when the transfer is complete
                 when the callback comes in the routine registered by driver info
                 table at the top of this file.
                ******************************************************************/

                 while ((Interrupt_received_mouse == FALSE) &&
                       (hid_device_mouse.DEV_STATE != USB_DEVICE_DETACHED))
                 {;}

                 /*print the data received from the mouse */

                 /*****************************************************
                 The following will print the data in numbers
                 ******************************************************/

                 /*****************************************************
                  if (buffer[0] & 0x01) printf("1 ");
                  else printf("0 ");
                  if (buffer[0] & 0x02) printf("1 ");
                  else printf("0 ");
                  if (buffer[0] & 0x04) printf("1 ");
                  else printf("0 ");

                  printf("X:%d ", buffer[1]);
                  printf("Y:%d ", buffer[2]);
                  printf("Z:%d ", buffer[3]);
                  printf("\n");
                  fflush(stdout);
                 ******************************************************/


                 /*****************************************************
                 The following will print the data in text strings
                 ******************************************************/


                 if (buffer[0] & 0x01) DEBUGMSG(USB_DBG_LEVEL,("Left Click "));
                 else DEBUGMSG(USB_DBG_LEVEL,("           "));
                 if (buffer[0] & 0x02) DEBUGMSG(USB_DBG_LEVEL,("Right Click "));
                 else DEBUGMSG(USB_DBG_LEVEL,("            "));
                 if (buffer[0] & 0x04) DEBUGMSG(USB_DBG_LEVEL,("Middle Click "));
                 else DEBUGMSG(USB_DBG_LEVEL,("             "));

                 if(buffer[1])
                 {
                    if(buffer[1] > 127) DEBUGMSG(USB_DBG_LEVEL,("Left  "));
                    else DEBUGMSG(USB_DBG_LEVEL,("Right "));
                 }
                 else { DEBUGMSG(USB_DBG_LEVEL,("      "));}

                 if(buffer[2])
                 {
                    if(buffer[2] > 127) DEBUGMSG(USB_DBG_LEVEL,("UP   "));
                    else DEBUGMSG(USB_DBG_LEVEL,("Down "));
                 }
                 else { DEBUGMSG(USB_DBG_LEVEL,("     "));}
                 if(buffer[3])
                 {
                    if(buffer[3] > 127) DEBUGMSG(USB_DBG_LEVEL,("Wheel Down"));
                    else DEBUGMSG(USB_DBG_LEVEL,("Wheel UP  "));
                 }
                 else { DEBUGMSG(USB_DBG_LEVEL,("          "));}
                 DEBUGMSG(USB_DBG_LEVEL,("\n"));
                 Interrupt_received_mouse = FALSE;
               } //end if pipe
            }
            break;
         case USB_DEVICE_DETACHED:
        	DEBUGMSG(USB_DBG_LEVEL,("\nHid device detached\n"));
            hid_device_mouse.DEV_STATE = USB_DEVICE_IDLE;
            break;
         case USB_DEVICE_OTHER:
            break;
         default:
        	DEBUGMSG(USB_DBG_LEVEL,("Unknown HID Device State = %d\n", hid_device_mouse.DEV_STATE));
            break;
      } /* End switch */
   } /* End for */
} /* End body */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_mouse_event
* Returned Value : None
* Comments       :
*     Called when HID device has been attached, detached, etc.
*END*--------------------------------------------------------------------*/

void usb_host_hid_mouse_event
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing call back */
      uint_32                          event_code
   )
{ /* Body */
   INTERFACE_DESCRIPTOR_PTR   intf_ptr =
      (INTERFACE_DESCRIPTOR_PTR)intf_handle;

   switch (event_code) {
      case USB_CONFIG_EVENT:
         /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT:
         /******************************************************
         This code is being executed under an attach interrupt
         and so Print the following only if printing is allowed
         under an interrupt.
         ******************************************************/

         #if 0
            printf("State = %d", hid_device_mouse.DEV_STATE);
            printf("  Class = %d", intf_ptr->bInterfaceClass);
            printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
            printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
            fflush(stdout);
         #endif

         if (hid_device_mouse.DEV_STATE == USB_DEVICE_IDLE) {
            hid_device_mouse.DEV_HANDLE = dev_handle;
            hid_device_mouse.INTF_HANDLE = intf_handle;
            hid_device_mouse.DEV_STATE = USB_DEVICE_ATTACHED;
         } else {
        	DEBUGMSG(USB_DBG_LEVEL,("HID device already attached\n"));
         } /* End if */
         break;
      case USB_INTF_EVENT:
         hid_device_mouse.DEV_STATE = USB_DEVICE_INTERFACED;
         break ;
      case USB_DETACH_EVENT:
         /******************************************************
         This code is being executed under an attach interupt
         and so Print the following only if printing is allowed
         under an interrupt.
         ******************************************************/

         #if 0
            /* Use only the interface with desired protocol */
            printf("----- Detach Event -----\n");
            printf("State = %d", hid_device.DEV_STATE);
            printf("  Class = %d", intf_ptr->bInterfaceClass);
            printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
            printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
            fflush(stdout);
         #endif

         if (hid_device_mouse.DEV_STATE == USB_DEVICE_INTERFACED) {
            hid_device_mouse.DEV_HANDLE = NULL;
            hid_device_mouse.INTF_HANDLE = NULL;
            hid_device_mouse.DEV_STATE = USB_DEVICE_DETACHED;
         } else {
        	DEBUGMSG(USB_DBG_LEVEL,("HID Device Not Attached\n"));
         } /* End if */
         break;
      default:
    	 DEBUGMSG(USB_DBG_LEVEL,("HID Device state = %d??\n", hid_device_mouse.DEV_STATE));
         hid_device_mouse.DEV_STATE = USB_DEVICE_IDLE;
         break;
   } /* EndSwitch */

} /* End body */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_user_callback
* Returned Value : None
* Comments       :
*     Called when a command is completed
*END*--------------------------------------------------------------------*/
static void usb_host_hid_mouse_user_callback
   (
      /* [IN] pointer to pipe */
      _usb_pipe_handle  pipe_handle,

      /* [IN] user-defined parameter */
      pointer           user_parm,

      /* [IN] buffer address */
      uchar_ptr         buffer,

      /* [IN] length of data transferred */
      uint_32           buflen,

      /* [IN] status, hopefully USB_OK or USB_DONE */
      uint_32           status
   )
{ /* Body */

   bHidCallBack = TRUE;

} /* End body */
/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_ctrl_callback
* Returned Value : None
* Comments       :
*     Called when a command is completed
*END*--------------------------------------------------------------------*/

void usb_host_hid_mouse_ctrl_callback
   (
      /* [IN] pointer to pipe */
      _usb_pipe_handle  pipe_handle,

      /* [IN] user-defined parameter */
      pointer           user_parm,

      /* [IN] buffer address */
      uchar_ptr         buffer,

      /* [IN] length of data transferred */
      uint_32           buflen,

      /* [IN] status, hopefully USB_OK or USB_DONE */
      uint_32           status
   )
{ /* Body */

   if (status) {
	   DEBUGMSG(USB_DBG_LEVEL,("\nHID Set_Protocol Request BOOT failed!: %x", status));
      exit(1);
   } /* End if */

   hid_mouse_conf_phase++;

} /* End body */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_recv_callback
* Returned Value : None
* Comments       :
*     Called when data is received on a pipe
*END*--------------------------------------------------------------------*/

void usb_host_hid_mouse_recv_callback
   (
      /* [IN] pointer to pipe */
      _usb_pipe_handle  pipe_handle,

      /* [IN] user-defined parameter */
      pointer           user_parm,

      /* [IN] buffer address */
      uchar_ptr         buffer,

      /* [IN] length of data transferred */
      uint_32           buflen,

      /* [IN] status, hopefully USB_OK or USB_DONE */
      uint_32           status
   )
{ /* Body */
   Interrupt_received_mouse = TRUE;
   if ((status != USB_OK) | (buflen < 3))
   {
	   DEBUGMSG(USB_DBG_LEVEL,("\nTransfer failed!: %x", status));
   }

}



/* EOF */

