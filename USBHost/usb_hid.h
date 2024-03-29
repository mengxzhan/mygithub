#ifndef __usb_hid_h__
#define __usb_hid_h__
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
** $Date: 2013/03/12 10:23:15 $
** $Revision: 1.1 $
*******************************************************************************
*** Description:
***   This file defines a template structure for Class Drivers.
***
**************************************************************************
**END*********************************************************/



/* System Include files */
#include "usb.h"
#include "hostapi.h"
#include "host_ch9.h"
#include "host_dev_list.h"
#include "host_common.h"
#include "usbprv.h"
#include "usb_hid_vers.h"


#define  USB_CLASS_HID              3
#define  USB_SUBCLASS_HID_NONE      0
#define  USB_SUBCLASS_HID_BOOT      1
#define  USB_PROTOCOL_HID_NONE      0
#define  USB_PROTOCOL_HID_KEYBOARD  1
#define  USB_PROTOCOL_HID_MOUSE     2

#define  USB_HID_PROTOCOL_BOOT       0
#define  USB_HID_PROTOCOL_REPORT     1

#define LONG_ITEM                      0xFE
#define SHORT_ITEM                     0x03

#define MAIN_INPUT                     0x80
#define MAIN_OUTPUT                    0x90
#define MAIN_FEATURE                   0xB0
#define MAIN_COLLECTION                0xA0
#define MAIN_END_COLLECTION            0xC0

#define GLOBAL_USAGE_PAGE              0x04
#define GLOBAL_LOGICAL_MIN             0x14
#define GLOBAL_LOGICAL_MAX             0x24
#define GLOBAL_PHYSICAL_MIN            0x34
#define GLOBAL_PHYSICAL_MAX            0x44
#define GLOBAL_UNIT_COMPONENT          0x54
#define GLOBAL_UNIT                    0x64
#define GLOBAL_REPORT_SIZE             0x74
#define GLOBAL_REPORT_ID               0x84
#define GLOBAL_REPORT_COUNT            0x94
#define GLOBAL_PUSH                    0xA4
#define GLOBAL_POP                     0xB4

#define LOCAL_USAGE                    0x08
#define LOCAL_USAGE_MINIMUM            0x18
#define LOCAL_USAGE_MAXIMUM            0x28
#define LOCAL_DESIGNATOR_INDEX         0x38
#define LOCAL_DESIGNATOR_MIN           0x48
#define LOCAL_DESIGNATOR_MAX           0x58
#define LOCAL_STRING_INDEX             0x78
#define LOCAL_STRING_MIN               0x88
#define LOCAL_STRING_MAX               0x98
#define LOCAL_SET_DELIMITER            0xA8


#define USAGE_PAGE_UNDEFINED           0x00
#define USAGE_PAGE_GENERIC_DESKTOP     0x01
#define USAGE_PAGE_KEYBOARD            0x07
#define USAGE_PAGE_LEDS                0x08
#define USAGE_PAGE_BUTTONS             0x09

#define DESKTOP_USAGE_POINTER          0x01
#define DESKTOP_USAGE_MOUSE            0x02
#define DESKTOP_USAGE_KEYBOARD         0x06
#define DESKTOP_USAGE_X                0x30
#define DESKTOP_USAGE_Y                0x31
#define DESKTOP_USAGE_Z                0x32
#define DESKTOP_USAGE_WHEEL            0x38

#define INPUT_DATA_CONSTBIT            0x01
#define INPUT_DATA_VAR_BIT             0x02
#define INPUT_ABS_RELATIVE_BIT         0x04
#define INPUT_NO_WRAP_BIT              0x08
#define INPUT_NON_LINEAR_BIT           0x10
#define INPUT_PREF_STATE_NO_PREF_BIT   0x20
#define INPUT_NO_NULL_STATE_BIT        0x40
#define INPUT_BIT_FIELD_BUF_BYTES_BIT  0x100


#define   GET_REPORT                   0x01
#define   GET_IDLE                     0x02
#define   GET_PROTOCOL                 0x03
#define   SET_REPORT                   0x09
#define   SET_IDLE                     0x0A
#define   SET_PROTOCOL                 0x0B

#define _PTR_      *
#define _CODE_PTR_ *

typedef void (_CODE_PTR_  HID_CALLBACK_USER_FN)
   (
      uint_32,                     /* [IN] Callback parameter */
      pointer,                     /* [IN] Pointer to class interface */
      pointer,                     /* [IN] Pointer to the data buffer */
      uint_32                      /* [IN] Error code */
   );


/* structure for HID class descriptor */
typedef struct _HID_DESCRIPTOR_STRUCT {
    uint_8     BLENGTH;
    uint_8     BDESCRIPTORTYPE;
    uint_8     BCDHID[2];
    uint_8     BCOUNTRYCODE;
    uint_8     BNUMDESCRIPTORS;
    uint_8     BCLASSDESCRIPTORTYPE;
    uint_8     BDESCRIPTORLENGTH[2];
} HID_DESCRIPTOR_STRUCT, _PTR_ HID_DESCRIPTOR_STRUCT_PTR;


/*
** HID Class Interface structure. This structure will be passed to
** all commands to this class driver.
*/

typedef struct {
   /* Each class must start with a GENERAL_CLASS struct */
   GENERAL_CLASS                             G;

   /* Pipes */
   _usb_pipe_handle                          P_CONTROL;
   _usb_pipe_handle                          P_INT_IN;
   _usb_pipe_handle                          P_INT_OUT;

   /* Only 1 command can be issued at one time */
   boolean                                   IN_SETUP;

   /* Higher level callback and parameter */
   tr_callback                               USER_CALLBACK;
   pointer                                   USER_PARAM;

   uint_8                                    IFNUM;

} USB_HID_CLASS_INTF_STRUCT, _PTR_ USB_HID_CLASS_INTF_STRUCT_PTR;


typedef struct {
   CLASS_CALL_STRUCT_PTR   CLASS_PTR;
   tr_callback             CALLBACK_FN;
   pointer                 CALLBACK_PARAM;
} HID_COMMAND, _PTR_ HID_COMMAND_PTR;


/* Class specific functions exported by HID class driver */
#ifdef __cplusplus
extern "C" {
#endif

extern void usb_class_hid_init(PIPE_BUNDLE_STRUCT_PTR, CLASS_CALL_STRUCT_PTR);
extern USB_STATUS usb_class_hid_get_idle(HID_COMMAND_PTR, uint_8, uint_8_ptr);
extern USB_STATUS usb_class_hid_get_protocol(HID_COMMAND_PTR, uchar_ptr);
extern USB_STATUS usb_class_hid_set_protocol(HID_COMMAND_PTR, uint_8);
extern USB_STATUS usb_class_hid_get_report(HID_COMMAND_PTR, uint_8, uint_8,
   pointer, uint_16);
extern USB_STATUS usb_class_hid_set_idle(HID_COMMAND_PTR, uint_8, uint_8);
extern USB_STATUS usb_class_hid_set_report(HID_COMMAND_PTR, uint_8, uint_8,
   pointer, uint_16);

#ifdef __cplusplus
}
#endif

#endif
