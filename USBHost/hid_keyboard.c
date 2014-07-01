/*******************************************************************************
** File          : $HeadURL$
** Author        : $Author: tracyhe $
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
** $Date: 2013/03/12 10:23:14 $
** $Revision: 1.1 $
*******************************************************************************
*** Description:
***   This file is an example of device drivers for the HID class. This example
demonstrates the keyboard functionality. Note that a real keyboard driver also
needs to distinguish between intentionally repeated and unrepeated key presses.
This example simply demonstrates how to receive data from a USB Keyboard.
Interpretation of data is upto the application and customers can add the code
for it.
***
**************************************************************************
**END*********************************************************/

#include "hid_keyboard.h"
#include "usb_hid.h"

/* skip the inclusion in dependency statge */
#ifndef __NO_SETJMP
   #include "stdio.h"
#endif
#include "stdlib.h"
#include "BU_interrupt.h"
#include "BU_SWconf.h"
extern void * USB_Uncached_memalloc(uint_32 n);

/***************************************
**
** Globals
*/
volatile boolean Interrupt_received = FALSE;
volatile boolean Set_protocol_done = FALSE;


/* Table of driver capabilities this application wants to use */
static  USB_HOST_DRIVER_INFO DriverInfoTable[] =
{
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_HID,                /* Class code                       */
      USB_SUBCLASS_HID_BOOT,        /* Sub-Class code                   */
      USB_PROTOCOL_HID_KEYBOARD,    /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_hid_keyboard_event   /* Application call back function   */
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

volatile DEVICE_STRUCT  hid_device = { 0 };
volatile uint_32        hid_conf_phase = 0;
_usb_host_handle        host_handle_keyboard;

#define MAX_SUPPORTED_USAGE_ID 40

static uchar HID_table[][2] = {
   { 0 , 0 },          /* Usage ID  0 */
   { 0 , 0 },          /* Usage ID  1 */
   { 0 , 0 },          /* Usage ID  2 */
   { 0 , 0 },          /* Usage ID  3 */
   { 0x61, 'A' },       /* Usage ID  4 */
   { 'b', 'B' },       /* Usage ID  5 */
   { 'c', 'C' },       /* Usage ID  6 */
   { 'd', 'D' },       /* Usage ID  7 */
   { 'e', 'E' },       /* Usage ID  8 */
   { 'f', 'F' },       /* Usage ID  9 */
   { 'g', 'G' },       /* Usage ID 10 */
   { 'h', 'H' },       /* Usage ID 11 */
   { 'i', 'I' },       /* Usage ID 12 */
   { 'j', 'J' },       /* Usage ID 13 */
   { 'k', 'K' },       /* Usage ID 14 */
   { 'l', 'L' },       /* Usage ID 15 */
   { 'm', 'M' },       /* Usage ID 16 */
   { 'n', 'N' },       /* Usage ID 17 */
   { 'o', 'O' },       /* Usage ID 18 */
   { 'p', 'P' },       /* Usage ID 19 */
   { 'q', 'Q' },       /* Usage ID 20 */
   { 'r', 'R' },       /* Usage ID 21 */
   { 's', 'S' },       /* Usage ID 22 */
   { 't', 'T' },       /* Usage ID 23 */
   { 'u', 'U' },       /* Usage ID 24 */
   { 'v', 'V' },       /* Usage ID 25 */
   { 'w', 'W' },       /* Usage ID 26 */
   { 'x', 'X' },       /* Usage ID 27 */
   { 'y', 'Y' },       /* Usage ID 28 */
   { 'z', 'Z' },       /* Usage ID 29 */
   { '1', '!' },       /* Usage ID 30 */
   { '2', '@' },       /* Usage ID 31 */
   { '3' ,'#' },       /* Usage ID 32 */
   { '4', '$' },       /* Usage ID 33 */
   { '5', '%' },       /* Usage ID 34 */
   { '6', '^' },       /* Usage ID 35 */
   { '7', '&' },       /* Usage ID 36 */
   { '8', '*' },       /* Usage ID 37 */
   { '9', '(' },       /* Usage ID 38 */
   { '0', ')' },       /* Usage ID 39 */
   { '\n', '\n' },     /* Usage ID 40 */
   { 0x1B, 0x1B},      /* Usage ID 41 */
   { 0x7F, 0x7F},      /* Usage ID 43 */
   { 0x9, 0x9 },       /* Usage ID 42 */
   { 0x20, 0x20 },     /* Usage ID 44 */

};
extern void * USB_Uncached_memalloc(uint_32 n);

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : main (Main_Task if using MQX)
* Returned Value : none
* Comments       :
*     Execution starts here
*
*END*--------------------------------------------------------------------*/

void Keyboard_test (void)

{ /* Body */
   USB_STATUS           status = USB_OK;
   _usb_pipe_handle     pipe;
   TR_INIT_PARAM_STRUCT tr;
   HID_COMMAND          hid_com;
   uchar_ptr            buffer;
   uchar_ptr            prevbuffer;
   uint_32              i,shift = 0;

   /* _usb_otg_init needs to be done with interrupts disabled */
   _disable_interrupts();

      /*
      ** It means that we are going to act like host, so we initialize the
      ** host stack. This call will allow USB system to allocate memory for
      ** data structures, it uses later (e.g pipes etc.).
      */
      status = _usb_host_init (
            HOST_CONTROLLER_NUMBER,   /* Use value in header file */
            MAX_FRAME_SIZE,            /* Frame size per USB spec  */
            &host_handle_keyboard);             /* Returned pointer */

      if (status != USB_OK)
      {
    	 DEBUGMSG(USB_DBG_LEVEL,("\nUSB Host Initialization failed. STATUS: %x", status));
      }

      /*
      ** since we are going to act as the host driver, register the driver
      ** information for wanted class/subclass/protocols
      */
      status = _usb_host_driver_info_register (
    		                           host_handle_keyboard,
                                       DriverInfoTable
                                       );
      if (status != USB_OK) {
    	  DEBUGMSG(USB_DBG_LEVEL,("\nDriver Registration failed. STATUS: %x", status));
      }

//   INT_Enable( INT_USB, 15 );
   IRQ_INT_Enable( INT_USB_OTG, 15 );
   /********************************************************
   Allocate two buffers for keeping the keyboard data.
   One for keeping a copy of previous data and other for
   receving the current data.
   ********************************************************/

   buffer = (uchar_ptr)USB_Uncached_memalloc(HID_BUFFER_SIZE);
   if (!buffer) {
	   DEBUGMSG(USB_DBG_LEVEL,("\nError allocating buffer!"));
      return;
   } /* End if */


   prevbuffer = (uchar_ptr)USB_Uncached_memalloc(HID_BUFFER_SIZE);
   if (!prevbuffer) {
	   DEBUGMSG(USB_DBG_LEVEL,("\nError allocating buffer!"));
	   return;
   } /* End if */
   /* make sure that previous copy of the data is zeroed out */
   USB_memzero(prevbuffer,HID_BUFFER_SIZE);

   /*
   ** Infinite loop, waiting for events requiring action
   */
   for ( ; ; ) {
      switch ( hid_device.DEV_STATE ) {
         case USB_DEVICE_IDLE:
            break;
         case USB_DEVICE_ATTACHED:
        	DEBUGMSG(USB_DBG_LEVEL,("\nHid device attached\n"));
            hid_device.DEV_STATE = USB_DEVICE_SET_INTERFACE_STARTED;
            status = _usb_hostdev_select_interface(hid_device.DEV_HANDLE,
               hid_device.INTF_HANDLE, (pointer)&hid_device.CLASS_INTF);
              if (status != USB_OK) {
                  DEBUGMSG(USB_DBG_LEVEL,("\nError in _usb_hostdev_select_interface: %x", status));
                  return;
              } /* End if */
            break;
         case USB_DEVICE_SET_INTERFACE_STARTED:
            break;
         case USB_DEVICE_INTERFACED:
            if (!hid_conf_phase)
            {
               hid_com.CLASS_PTR = (CLASS_CALL_STRUCT_PTR)&hid_device.CLASS_INTF;
               hid_com.CALLBACK_PARAM = 0;

               /*******************************************************
                Uncomment the following lines to force the keyboard to
                behave as in USB Hid class standard boot protocol.
               *******************************************************/
#if 1
               hid_com.CALLBACK_FN = usb_host_hid_ctrl_callback;
               hid_conf_phase = 1;

//               status = usb_class_hid_set_protocol(&hid_com, USB_HID_PROTOCOL_BOOT);
               status = usb_class_hid_set_idle(&hid_com,0,0);

               if (status != USB_STATUS_TRANSFER_QUEUED)
               {
            	   DEBUGMSG(USB_DBG_LEVEL,("\nError in usb_class_hid_set_protocol: %x", status));
               } /* End if */

               /* wait until control pipe request is done */
               while(Set_protocol_done == FALSE) {;}
#else
               hid_conf_phase = 2;

               usb_class_hid_set_idle(&hid_com,0,0);
#endif

            } else if (hid_conf_phase == 1) {
               /* Do nothing */
            } else {

               pipe = _usb_hostdev_find_pipe_handle(hid_device.DEV_HANDLE,
                  hid_device.INTF_HANDLE, USB_INTERRUPT_PIPE, USB_RECV);

               if (pipe) {

                  /******************************************************************
                  Initiate a transfer request on the interrupt pipe
                  ******************************************************************/

                  usb_hostdev_tr_init(&tr, usb_host_hid_recv_callback, NULL);
                  tr.RX_BUFFER = buffer;
                  tr.RX_LENGTH = HID_BUFFER_SIZE;


                  status = _usb_host_recv_data(host_handle_keyboard, pipe, &tr);
                  if (status != USB_STATUS_TRANSFER_QUEUED) {
                	 DEBUGMSG(USB_DBG_LEVEL,("\nError in _usb_host_recv_data: %x", status));
                     return;
                  } /* End if */


                  /******************************************************************
                   Wait in a loop till we get the data from keyboard. Note that this
                   is not necessary to loop and an application could proceed and
                   do something else. It will notified when the transfer is complete
                   when the callback comes in the routine registered by driver info
                   table at the top of this file.
                  ******************************************************************/

                  while ((Interrupt_received == FALSE) &&
                        (hid_device.DEV_STATE != USB_DEVICE_DETACHED))
                  {;}

                  Interrupt_received = FALSE;

                  /******************************************************************
                  The way keyboard works is that it sends reports of 8 bytes of data
                  every time keys are pressed. However, it reports all the keys
                  that are pressed in a single report. This means that every time
                  we get a report, we should compare it with previous report and
                  print only the changed keys. When a user is typing very fast, he is
                  pressing new keys before previous one is released. So we should
                  be smart enough to detect that previous one is still pressed. This
                  will show as smoother output on screen. The following code could
                  really be implemented by a user in the way he would like it to be.
                  ******************************************************************/

                  shift = 0;

                  /*compare with the previous buffer all 8 bytes*/

                  if(buffer[0] != prevbuffer[0])
                  {
                      /* Check Modifiers in byte 0 (see HID specification 1.11 page 56) */
                      if       (buffer[0] & 0x01) { DEBUGMSG(USB_DBG_LEVEL,("LEFT CTRL "));              }
                      else if  (buffer[1] & 0x02) { DEBUGMSG(USB_DBG_LEVEL,("LEFT SHIFT ")); shift = 1;  }
                      else if  (buffer[1] & 0x04) { DEBUGMSG(USB_DBG_LEVEL,("LEFT ALT "));               }
                      else if  (buffer[1] & 0x08) { DEBUGMSG(USB_DBG_LEVEL,("LEFT GUI "));               }
                      else if  (buffer[1] & 0x10) { DEBUGMSG(USB_DBG_LEVEL,("RIGHT CTRL "));             }
                      else if  (buffer[1] & 0x20) { DEBUGMSG(USB_DBG_LEVEL,("RIGHT SHIFT ")); shift = 1; }
                      else if  (buffer[1] & 0x40) { DEBUGMSG(USB_DBG_LEVEL,("RIGHT ALT "));              }
                      else if  (buffer[1] & 0x80) { DEBUGMSG(USB_DBG_LEVEL,("RIGHT GUI "));              }
                  }

                   /* Byte 1 is reserved (HID specification 1.11 page 60) */

                   /*
                   ** Check Keybcodes in bytes 2 to 7 (HID spec 1.11 page 60), and HID Usage
                   ** Tables "Key Codes" section
                   */
                   for (i = 2; i < HID_BUFFER_SIZE; i++)
                   {
                      if ((buffer[i] != prevbuffer[i]) &&
                         (buffer[i] <= MAX_SUPPORTED_USAGE_ID))
                      {
                         if (HID_table[buffer[i]][shift])
                         {
                        	 DEBUGMSG(USB_DBG_LEVEL,(&HID_table[buffer[i]][shift]));
                         }
                      }
                   }
                /* copy the current buffer in previous one */
                USB_memcopy(buffer,prevbuffer,HID_BUFFER_SIZE);

               } /* End if */

            } /* End if */
            break;
         case USB_DEVICE_DETACHED:
        	DEBUGMSG(USB_DBG_LEVEL,("\nHid device detached\n"));
            hid_device.DEV_STATE = USB_DEVICE_IDLE;
            break;
         case USB_DEVICE_OTHER:
            break;
         default:
        	DEBUGMSG(USB_DBG_LEVEL,("Unknown Printer Device State = %d\n", hid_device.DEV_STATE));
            break;
      } /* End switch */
   } /* End for */
} /* End body */




/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_keyboard_event
* Returned Value : None
* Comments       :
*     Called when HID device has been attached, detached, etc.
*END*--------------------------------------------------------------------*/

void usb_host_hid_keyboard_event
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing call back */
      uint_32                          event_code
   )
{ /* Body */


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
            fflush(stdout);
            printf("State = %d", hid_device.DEV_STATE);
            printf("  Class = %d", intf_ptr->bInterfaceClass);
            printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
            printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
            fflush(stdout);
         #endif
         if (hid_device.DEV_STATE == USB_DEVICE_IDLE) {
            hid_device.DEV_HANDLE = dev_handle;
            hid_device.INTF_HANDLE = intf_handle;
            hid_device.DEV_STATE = USB_DEVICE_ATTACHED;
         } else {
        	DEBUGMSG(USB_DBG_LEVEL,("HID device already attached\n"));
         } /* End if */
         break;
      case USB_INTF_EVENT:
    	 DEBUGMSG(USB_DBG_LEVEL,("device interfaced\n"));
         hid_device.DEV_STATE = USB_DEVICE_INTERFACED;
         break ;
      case USB_DETACH_EVENT:
         /******************************************************
         This code is being executed under an attach interrupt
         and so Print the following only if printing is allowed
         under an interrupt.
         ******************************************************/

         #if 0
         /* Use only the interface with desired protocol */
         printf("----- Detach Event -----\n");
         fflush(stdout);
         printf("State = %d", hid_device.DEV_STATE);
         printf("  Class = %d", intf_ptr->bInterfaceClass);
         printf("  SubClass = %d", intf_ptr->bInterfaceSubClass);
         printf("  Protocol = %d\n", intf_ptr->bInterfaceProtocol);
         fflush(stdout);
         #endif

         if (hid_device.DEV_STATE == USB_DEVICE_INTERFACED) {
            hid_device.DEV_HANDLE = NULL;
            hid_device.INTF_HANDLE = NULL;
            hid_device.DEV_STATE = USB_DEVICE_DETACHED;
         } else {
        	 DEBUGMSG(USB_DBG_LEVEL,("HID Device Not Attached\n"));
         } /* End if */
         break;
      default:
    	 DEBUGMSG(USB_DBG_LEVEL,("HID Device state = %d??\n", hid_device.DEV_STATE));
         hid_device.DEV_STATE = USB_DEVICE_IDLE;
         break;
   } /* End Switch */
} /* End body */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_ctrl_callback
* Returned Value : None
* Comments       :
*     Called when a control pipe command is completed. Note that it is upto
the user to implement this routine. This routine currently just sets a boolean
variable to signal that control transfer is complete. This is because this example
program can also run with no operating system support. In an OS environment
a user can signal an event or decrease semaphore or signal a message etc. when
a transfer is complete.
*END*--------------------------------------------------------------------*/

void usb_host_hid_ctrl_callback
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
      return;
   } /* End if */
   Set_protocol_done = TRUE;
   hid_conf_phase++;
} /* End body */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hid_recv_callback
* Returned Value : None
* Comments       :
*     Called when a interrupt pipe transfer is completed. Note that it is upto
the user to implement this routine. This routine currently just sets a boolean
variable to signal that interrupt transfer is complete. This is because this example
program can also run with no operating system support. In an OS environment
a user can signal an event or decrease semaphore or signal a message etc. when
a transfer is complete.
*END*--------------------------------------------------------------------*/

void usb_host_hid_recv_callback
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
      Interrupt_received = TRUE;
} /* End body */

/* EOF */

