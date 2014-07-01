#ifndef __hid_mouse_h__
#define __hid_mouse_h__
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
*** Comments:
***   This file contains printer-application types and definitions.
***
**************************************************************************
**END*********************************************************/


#include "hostapi.h"



/***************************************
**
** Application-specific definitions
*/

/* Used to initialize USB controller */
#define MAX_FRAME_SIZE           1024
#define HOST_CONTROLLER_NUMBER      0

#define HID_BUFFER_SIZE                   6

#define  USB_DEVICE_IDLE                   (0)
#define  USB_DEVICE_ATTACHED               (1)
#define  USB_DEVICE_CONFIGURED             (2)
#define  USB_DEVICE_SET_INTERFACE_STARTED  (3)
#define  USB_DEVICE_INTERFACED             (4)
#define  USB_DEVICE_DETACHED               (5)
#define  USB_DEVICE_OTHER                  (6)


/*
** Following structs contain all states and pointers
** used by the application to control/operate devices.
*/

typedef struct device_struct {
   uint_32                          DEV_STATE;  /* Attach/detach state */
   _usb_device_instance_handle      DEV_HANDLE;
   _usb_interface_descriptor_handle INTF_HANDLE;
   CLASS_CALL_STRUCT                CLASS_INTF; /* Class-specific info */
} DEVICE_STRUCT,  _PTR_ DEVICE_STRUCT_PTR;


/* Alphabetical list of Function Prototypes */

#ifdef __cplusplus
extern "C" {
#endif


void usb_host_hid_mouse_recv_callback(_usb_pipe_handle, pointer, uchar_ptr, uint_32,
   uint_32);
void usb_host_hid_mouse_ctrl_callback(_usb_pipe_handle, pointer, uchar_ptr, uint_32,
   uint_32);
void usb_host_hid_mouse_user_callback(_usb_pipe_handle, pointer, uchar_ptr, uint_32,
   uint_32);
void usb_host_hid_mouse_event(_usb_device_instance_handle,
   _usb_interface_descriptor_handle, uint_32);

#ifdef __cplusplus
}
#endif


#endif

/* EOF */





