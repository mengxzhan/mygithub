#ifndef __usb_desc_h__
#define __usb_desc_h__
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
***   This file contains struct definitions for USB descriptors.
***                                                               
**************************************************************************
**END*********************************************************/
#include "usbh_types.h"

#define _PTR_      *
#define _CODE_PTR_ *
typedef struct usb_device_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 18 */
   uint_8   bDescriptorType;  /* DEVICE descriptor type = 1 */
   uint_8   bcdUSD[2];        /* USB spec in BCD, e.g. 0x0200 */
   uint_8   bDeviceClass;     /* Class code, if 0 see interface */
   uint_8   bDeviceSubClass;  /* Sub-Class code, 0 if class = 0 */
   uint_8   bDeviceProtocol;  /* Protocol, if 0 see interface */
   uint_8   bMaxPacketSize;   /* Endpoint 0 max. size */
   uint_8   idVendor[2];      /* Vendor ID per USB-IF */
   uint_8   idProduct[2];     /* Product ID per manufacturer */
   uint_8   bcdDevice[2];     /* Device release # in BCD */
   uint_8   iManufacturer;    /* Index to manufacturer string */
   uint_8   iProduct;         /* Index to product string */
   uint_8   iSerialNumber;    /* Index to serial number string */
   uint_8   bNumConfigurations; /* Number of possible configurations */ 
} DEVICE_DESCRIPTOR, _PTR_ DEVICE_DESCRIPTOR_PTR;  

typedef struct usb_configuration_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 9 */
   uint_8   bDescriptorType;  /* CONFIGURATION type = 2 or 7 */
   uint_8   wTotalLength[2];  /* Length of concatenated descriptors */
   uint_8   bNumInterfaces;   /* Number of interfaces, this config. */
   uint_8   bConfigurationValue;  /* Value to set this config. */ 
   uint_8   iConfig;          /* Index to configuration string */
   uint_8   bmAttributes;     /* Config. characteristics */
   #define  CONFIG_RES7       (0x80)  /* Reserved, always = 1 */
   #define  CONFIG_SELF_PWR   (0x40)  /* Self-powered device */
   #define  CONFIG_WAKEUP     (0x20)  /* Remote wakeup */
   uint_8   bMaxPower;        /* Max.power from bus, 2mA units */
} CONFIGURATION_DESCRIPTOR, _PTR_ CONFIGURATION_DESCRIPTOR_PTR;  

typedef struct usb_interface_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 9 */
   uint_8   bDescriptorType;  /* INTERFACE descriptor type = 4 */
   uint_8   bInterfaceNumber; /* Interface no.*/
   uint_8   bAlternateSetting;  /* Value to select this IF */
   uint_8   bNumEndpoints;    /* Number of endpoints excluding 0 */
   uint_8   bInterfaceClass;  /* Class code, 0xFF = vendor */
   uint_8   bInterfaceSubClass;  /* Sub-Class code, 0 if class = 0 */
   uint_8   bInterfaceProtocol;  /* Protocol, 0xFF = vendor */
   uint_8   iInterface;       /* Index to interface string */
} INTERFACE_DESCRIPTOR, _PTR_ INTERFACE_DESCRIPTOR_PTR;  

typedef struct usb_endpoint_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 7 */
   uint_8   bDescriptorType;  /* ENDPOINT descriptor type = 5 */
   uint_8   bEndpointAddress; /* Endpoint # 0 - 15 | IN/OUT */
   #define  IN_ENDPOINT    (0x80)   /* IN endpoint, device to host */
   #define  OUT_ENDPOINT   (0x00)   /* OUT endpoint, host to device */
   #define  ENDPOINT_MASK  (0x0F)   /* Mask endpoint # */
   uint_8   bmAttributes;     /* Transfer type */
   #define  CONTROL_ENDPOINT  (0x00)   /* Control transfers */
   #define  ISOCH_ENDPOINT    (0x01)   /* Isochronous transfers */
   #define  BULK_ENDPOINT     (0x02)   /* Bulk transfers */
   #define  IRRPT_ENDPOINT    (0x03)   /* Interrupt transfers */
   #define  EP_TYPE_MASK      (0x03)   /* Mask type bits */
   /* Following must be zero except for isochronous endpoints */
   #define  ISOCH_NOSYNC      (0x00)   /* No synchronization */
   #define  ISOCH_ASYNC       (0x04)   /* Asynchronous */
   #define  ISOCH_ADAPT       (0x08)   /* Adaptive */
   #define  ISOCH_SYNCH       (0x0C)   /* Synchrounous */
   #define  ISOCH_DATA        (0x00)   /* Data endpoint */
   #define  ISOCH_FEEDBACK    (0x10)   /* Feedback endpoint */
   #define  ISOCH_IMPLICIT    (0x20)   /* Implicit feedback */
   #define  ISOCH_RESERVED    (0x30)   /* Reserved */
   uint_8   wMaxPacketSize[2];   /* Bits 10:0 = max. packet size */
   /* For high-speed interrupt or isochronous only, additional
   **   transaction opportunities per microframe follow.*/
   #define  PACKET_SIZE_MASK     (0x7FF)  /* packet size bits */
   #define  NO_ADDITONAL      (0x0000)   /* 1 / microframe */
   #define  ONE_ADDITIONAL    (0x0800)   /* 2 / microframe */
   #define  TWO_ADDITIONAL    (0x1000)   /* 3 / microframe */
   #define  ADDITIONAL_MASK   (ONE_ADDITIONAL | TWO_ADDITIONAL)
   uint_8   iInterval;        /* Polling interval in (micro) frames */
} ENDPOINT_DESCRIPTOR, _PTR_ ENDPOINT_DESCRIPTOR_PTR;  

typedef struct usb_qualifier_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 10 */
   uint_8   bDescriptorType;  /* DEVICE QUALIFIER type = 6 */
   uint_8   bcdUSD[2];        /* USB spec in BCD, e.g. 0x0200 */
   uint_8   bDeviceClass;     /* Class code, if 0 see interface */
   uint_8   bDeviceSubClass;  /* Sub-Class code, 0 if class = 0 */
   uint_8   bDeviceProtocol;  /* Protocol, if 0 see interface */
   uint_8   bMaxPacketSize;   /* Endpoint 0 max. size */
   uint_8   bNumConfigurations; /* Number of possible configurations */
   uint_8   bReserved;        /* Reserved = 0 */ 
} QUALIFIER_DESCRIPTOR, _PTR_ QUALIFIER_DESCRIPTOR_PTR;  

/* Other-Config type 7 fields are identical to type 2 above */

/* Interface-Power descriptor  type 8 not used  in this version */

typedef struct usb_otg_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 9 */
   uint_8   bDescriptorType;  /* CONFIGURATION type = 2 or 7 */
   uint_8   bmAttributes;     /* OTG characteristics */
   #define  OTG_SRP_SUPPORT   (0x01)  /* Supports SRP */
   #define  OTG_HNP_SUPPORT   (0x02)  /* Supports HNP */
} OTG_DESCRIPTOR, _PTR_ OTG_DESCRIPTOR_PTR;  

typedef union descriptor_union
{
   uint_32                       word;
   uchar_ptr                     bufr;
   pointer                       pntr;
   DEVICE_DESCRIPTOR_PTR         dvic;
   CONFIGURATION_DESCRIPTOR_PTR  cfig;
   INTERFACE_DESCRIPTOR_PTR      intf;
   ENDPOINT_DESCRIPTOR_PTR       ndpt;
   QUALIFIER_DESCRIPTOR_PTR      qual;
   OTG_DESCRIPTOR_PTR            otg;
}  DESCRIPTOR_UNION, _PTR_ DESCRIPTOR_UNION_PTR;
                           
/* Prototypes */

#ifdef __cplusplus
extern "C" {
#endif

extern uint_32 usb_host_init(uint_8, uint_32, 
                  _usb_host_handle _PTR_);
extern uint_32 _usb_host_open_pipe(_usb_host_handle, 
                  PIPE_INIT_PARAM_STRUCT_PTR, _usb_pipe_handle _PTR_ );

#ifdef __cplusplus
}
#endif                         

#endif

/* EOF */
