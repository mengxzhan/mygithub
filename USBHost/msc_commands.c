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
***   This file contains device driver for mass storage class. This code tests
***   the UFI set of commands.
***
**************************************************************************
**END*********************************************************/

/**************************************************************************
Include the OS and BSP dependent files that define IO functions and
basic types. You may like to change these files for your board and RTOS 
**************************************************************************/
/**************************************************************************
Include the USB stack header files.
**************************************************************************/
#include "usbh_types.h"
#ifdef __USB_OTG__
#include "otgapi.h"
#include "devapi.h"
#else
#include "hostapi.h"
#endif

#include "BU_interrupt.h"
#include "BU_SWconf.h"
#include "test_usb.h"

#ifdef __USB_OS_MQX__
   #include "mqx_arc.h"
   #include "bsp.h"
   #include "fio.h"
#else
#ifndef __NO_SETJMP
   #include "stdio.h"
#endif
   #include "stdlib.h"
#endif

#include "BU_Interrupt.h"

/**************************************************************************
Local header files for this application
**************************************************************************/
#include "msc_commands.h"

/**************************************************************************
Class driver files for this application
**************************************************************************/

#include "usb_mass_bo.h"
#include "usb_mass_ufi.h"
//#include "EFSL/Ls.h"
//#include "EFSL/efs.h"
#include "usb_hub_class.h"
extern void * USB_memalloc(uint_32 n);
extern volatile boolean Host_inited;
#define	USB_PRIORITY	15
extern volatile boolean test_flag;
/**************************************************************************
A driver info table defines the devices that are supported and handled
by file system application. This table defines the PID, VID, class and
subclass of the USB device that this application listens to. If a device
that matches this table entry, USB stack will generate a attach callback.
As noted, this table defines a UFI class device and a USB
SCSI class device (e.g. high-speed hard disk) as supported devices.
see the declaration of structure USB_HOST_DRIVER_INFO for details
or consult the software architecture guide.
**************************************************************************/


static  USB_HOST_DRIVER_INFO DriverInfoTable[] =
{
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer     */
      9,                            /* Class code for Hub               */
      0,                            /* Sub-Class code                   */
      0,                            /* Protocol 00: Full speed              */
      0,                            /* Reserved                         */
      usb_host_hub_device_event     /* Application call back function   */
   },
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer     */
      9,                            /* Class code for Hub               */
      0,                            /* Sub-Class code                   */
      1,                            /* Protocol 01: High speed Hub with single TT  */
      0,                            /* Reserved                         */
      usb_host_hub_device_event     /* Application call back function   */
   },
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer     */
      9,                            /* Class code for Hub               */
      0,                            /* Sub-Class code                   */
      2,                            /* Protocol 02: High Speed hub w/ multiple TTs   */
      0,                            /* Reserved                         */
      usb_host_hub_device_event     /* Application call back function   */
   },	
   /* Floppy drive */
   {
      {0x00,0x00},                  /* Vendor ID per USB-IF             */
      {0x00,0x00},                  /* Product ID per manufacturer      */
      USB_CLASS_MASS_STORAGE,       /* Class code                       */
      USB_SUBCLASS_MASS_UFI,        /* Sub-Class code                   */
      USB_PROTOCOL_MASS_BULK,       /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_mass_device_event    /* Application call back function   */
   },

   /* USB 2.0 hard drive */
   {

      {0x81,0x07},                  /* Vendor ID per USB-IF             */
      {0x71,0x55},                  /* Product ID per manufacturer      */
      USB_CLASS_MASS_STORAGE,       /* Class code                       */
      USB_SUBCLASS_MASS_SCSI,       /* Sub-Class code                   */
      USB_PROTOCOL_MASS_BULK,       /* Protocol                         */
      0,                            /* Reserved                         */
      usb_host_mass_device_event    /* Application call back function   */
   },

   {
      {0x00,0x00},                  /* All-zero entry terminates        */
      {0x00,0x00},                  /*    driver info list.             */
      0,
      0,
      0,
      0,
      NULL
   }

};

/**************************************************************************
   Global variables
**************************************************************************/
volatile DEVICE_STRUCT       mass_device = { 0 };   /* mass storage device struct */
volatile boolean bCallBack = FALSE;
volatile USB_STATUS bStatus       = USB_OK;
volatile uint_32  dBuffer_length = 0;
boolean  Attach_Notification = FALSE;
_usb_host_handle        host_handle;         /* global handle for calling host   */


/* the following is the mass storage class driver object structure. This is
used to send commands down to  the class driver. See the Class API document
for details */

COMMAND_OBJECT_PTR pCmd, pCmd1;

/*some handles for communicating with USB stack */
_usb_host_handle     host_handle; /* host controller status etc. */
extern boolean _usb_ehci_process_port_change
(
   /* [IN] the USB Host state structure */
   _usb_host_handle        handle
);
/**************************************************************************
The following is the way to define a multi tasking system in MQX RTOS.
Remove this code and use your own RTOS way of defining tasks (or threads).
**************************************************************************/

#ifdef __USB_OS_MQX__
#define MAIN_TASK          (10)
extern void Main_Task(uint_32 param);
TASK_TEMPLATE_STRUCT  MQX_template_list[] =
{
   { MAIN_TASK,      Main_Task,      3000L,  7L, "Main",      MQX_AUTO_START_TASK},
   { 0L,             0L,             0L,   0L, 0L,          0L }
};

#endif



/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : main
* Returned Value : none
* Comments       :
*     Execution starts here
*
*END*----------------------------------------------------------------------*/
int alloc_size;

#ifdef __USB_OS_MQX__
void Main_Task ( uint_32 param )
#else
void usb_host_main( uint_8 host_device_num)
#endif

{ /* Body */

   USB_STATUS           status = USB_OK;

   _disable_interrupts();

   alloc_size = 0;
	
   #ifdef __USB_OS_MQX__
      _int_install_unexpected_isr ( );
      status = _usb_driver_install(0, &_bsp_usb_callback_table);
      if (status) {
         printf("ERROR: %ls", status);
         exit(1);
      } /* End if */
   #endif

   status = _usb_host_init
      (host_device_num,    /* Use value in header file */
       MAX_FRAME_SIZE,            /* Frame size per USB spec  */
       &host_handle);             /* Returned pointer */

   if (status != USB_OK) {
	   DEBUGMSG(USB_DBG_LEVEL,("usb_host_init failed \n" ));
   } /* End if */

   /*
   ** Since we are going to act as the host driver, register
   ** the driver information for wanted class/subclass/protocols
   */
   status = _usb_host_driver_info_register(host_handle, DriverInfoTable);
   if (status != USB_OK) {
	   DEBUGMSG(USB_DBG_LEVEL,("_usb_host_driver_info_register failed \n" ));
   }

	if ( host_device_num == USB_OTG_PORT )
		IRQ_INT_Enable( INT_USB_OTG, 15 );
//		INT_Enable( INT_USB, USB_PRIORITY );
	if ( host_device_num == USB_HSIC1_PORT )
		IRQ_INT_Enable( INT_USB_HSIC1, 15 );
//		INT_Enable( INT_USB_HSIC1, USB_PRIORITY );

	if(host_device_num == 0) {
	    mass_device.dev_state = 0;
	    mass_device.pbs = NULL;			/* Device & pipes */
	    mass_device.dev_handle = NULL;
	    mass_device.intf_handle = NULL;
	    mass_device.class_intf.class_intf_handle = NULL; /* Class-specific info */
	    mass_device.class_intf.code_key = 0; /* Class-specific info */

	    bCallBack = FALSE;
	    bStatus       = USB_OK;
	    dBuffer_length = 0;
	    Attach_Notification = FALSE;
        /*----------------------------------------------------**
        ** Infinite loop, waiting for events requiring action **
        **----------------------------------------------------*/
       for ( ; ; ) {
           switch ( mass_device.dev_state ) {
               case USB_DEVICE_IDLE:
               break ;
               case USB_DEVICE_ATTACHED:
                   DEBUGMSG(USB_DBG_LEVEL,("Mass Storage Attached\n" ));
                   mass_device.dev_state = USB_DEVICE_SET_INTERFACE_STARTED;
                   status = _usb_hostdev_select_interface(mass_device.dev_handle,
                   mass_device.intf_handle, (pointer)&mass_device.class_intf);
               break ;
               case USB_DEVICE_SET_INTERFACE_STARTED:
               break;
               case USB_DEVICE_INTERFACED:
                   DEBUGMSG(USB_DBG_LEVEL,("Mass Storage Interfaced, Enter MSD test\r\n" ));
                   usb_host_mass_test_storage();
                   mass_device.dev_state = USB_DEVICE_OTHER;
			       return;
			
               case USB_DEVICE_DETACHED:
                   DEBUGMSG(USB_DBG_LEVEL,("Mass Storage Detached\n" ));
                   mass_device.dev_state = USB_DEVICE_IDLE;
               break;
               case USB_DEVICE_OTHER:
               break ;
               default:
//             DEBUGMSG(USB_DBG_LEVEL, ("Unknown Mass Storage Device State = %d\n",
//               mass_device.dev_state );

               break ;
            } /* End switch */
        } /* End for */
	} else if (host_device_num == 1) {
		hub_test();
	}

} /* End body */



/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_mass_device_event
* Returned Value : None
* Comments       :
*     called when mass storage device has been attached, detached, etc.
*END*--------------------------------------------------------------------*/
static void usb_host_mass_device_event
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing callback */
      uint_32           event_code
   )
{ /* Body */
   /* clean warning INTERFACE_DESCRIPTOR_PTR   intf_ptr =
      (INTERFACE_DESCRIPTOR_PTR)intf_handle; */

   switch (event_code) {
      case USB_CONFIG_EVENT:
         /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT:
         if (mass_device.dev_state == USB_DEVICE_IDLE) {
            mass_device.dev_handle = dev_handle;
            mass_device.intf_handle = intf_handle;
            mass_device.dev_state = USB_DEVICE_ATTACHED;
         } else {
            DEBUGMSG(USB_DBG_LEVEL,("Mass Storage Already Attached\n"));
         } /* EndIf */
         break;
      case USB_INTF_EVENT:
         mass_device.dev_state = USB_DEVICE_INTERFACED;
         break;
      case USB_DETACH_EVENT:
         if (mass_device.dev_state == USB_DEVICE_INTERFACED) {
            mass_device.dev_handle = NULL;
            mass_device.intf_handle = NULL;
            mass_device.dev_state = USB_DEVICE_DETACHED;
         } else {
            DEBUGMSG(USB_DBG_LEVEL,("Mass Storage Not Attached\n"));
         } /* EndIf */
         break;
      default:
         mass_device.dev_state = USB_DEVICE_IDLE;
         break;
   } /* EndSwitch */
} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hub_device_event_esc
* Returned Value : None
* Comments       :
*     called when mass storage device has been attached, detached, etc.
*END*--------------------------------------------------------------------*/
static void usb_host_hub_device_event_esc
   (
      /* [IN] pointer to device instance */
      _usb_device_instance_handle      dev_handle,

      /* [IN] pointer to interface descriptor */
      _usb_interface_descriptor_handle intf_handle,

      /* [IN] code number for event causing callback */
      uint_32           event_code
   )
{ /* Body */

   switch (event_code) {
      case USB_CONFIG_EVENT:
         /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT:
         if (mass_device.dev_state == USB_DEVICE_IDLE) {
            mass_device.dev_handle = dev_handle;
            mass_device.intf_handle = intf_handle;
            mass_device.dev_state = USB_DEVICE_ATTACHED;
         } else {
            DEBUGMSG(USB_DBG_LEVEL,("USB Hub Already Attached\n"));
         } /* EndIf */
         break;
      case USB_INTF_EVENT:
         mass_device.dev_state = USB_DEVICE_INTERFACED;
         DEBUGMSG(USB_DBG_LEVEL,("USB Hub Already Interfaced\n"));
         break;
      case USB_DETACH_EVENT:
         if (mass_device.dev_state == USB_DEVICE_INTERFACED) {
            mass_device.dev_handle = NULL;
            mass_device.intf_handle = NULL;
            mass_device.dev_state = USB_DEVICE_DETACHED;
         } else {
            DEBUGMSG(USB_DBG_LEVEL,("USB Hub Already Not Attached\n"));
         } /* EndIf */
         break;
      default:
         mass_device.dev_state = USB_DEVICE_IDLE;
         DEBUGMSG(USB_DBG_LEVEL,("USB Hub Not Attached\n"));
         break;
   } /* EndSwitch */
} /* End body */


/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_mass_ctrl_calback
* Returned Value : None
* Comments       :
*     called on completion of a control-pipe transaction.
*
*END*--------------------------------------------------------------------*/

static void usb_host_mass_ctrl_callback
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
   bCallBack = TRUE;
   bStatus = status;
} /* End body */


void callback_bulk_pipe
   (
      /* [IN] Status of this command */
      USB_STATUS status,

      /* [IN] pointer to USB_MASS_BULK_ONLY_REQUEST_STRUCT*/
      pointer p1,

      /* [IN] pointer to the command object*/
      pointer  p2,

      /* [IN] Length of data transmitted */
      uint_32 buffer_length
   )
{ /* Body */
   DEBUGMSG(USB_DBG_LEVEL,("Callback\n"));
   dBuffer_length = buffer_length;
   bCallBack = TRUE;
   bStatus = status;

} /* End body */

/*FUNCTION*----------------------------------------------------------------
 *
 * Function Name : usb_host_mass_fs_test
 * Return Value  
 * Comments :
 *    Using open source FS
 * end*--------------------------------------------------------------------*/
/*static int usb_host_mass_fs_test
    (
        void
    )
{
	uint8_t filenameString[15]  = {0};
	EmbeddedFileSystem          efs;
	EmbeddedFile                file;
	DirList                     list;
	uint8_t textFileBuffer[50] = "Marvell test usb mass storage \r\n" ;
	if(efs_init(&efs,0)!=0)
	{*/
	    /* efs initialisation fails*/
/*	    DEBUGMSG(TTC_DBG_LEVEL,("EFS_INIT Failed \r"));
	    return -1;
	}
	if (ls_openDir(&list, &(efs.myFs), "/") != 0)
	{*/
	    /*Could not open the selected directory*/
/*		DEBUGMSG(TTC_DBG_LEVEL, ("ls_openDir failed \r"));
	    return(-2);
	}
	while(ls_getNext(&list) == 0)
	{
	          
	      filenameString[0]  = list.currentEntry.FileName[0];
	      filenameString[1]  = list.currentEntry.FileName[1];
	      filenameString[2]  = list.currentEntry.FileName[2];
	      filenameString[3]  = list.currentEntry.FileName[3];
	      filenameString[4]  = list.currentEntry.FileName[4];
	      filenameString[5]  = list.currentEntry.FileName[5];
	      filenameString[6]  = list.currentEntry.FileName[6];
	      filenameString[7]  = list.currentEntry.FileName[7];
	      filenameString[8]  = '.' ;
	      filenameString[9]  = list.currentEntry.FileName[8];
	      filenameString[10] = list.currentEntry.FileName[9];
	      filenameString[11] = list.currentEntry.FileName[10];
	      DEBUGMSG(TTC_DBG_LEVEL, ("\r\n ls_getNext \r\n"));
	      DEBUGMSG(TTC_DBG_LEVEL, (filenameString));
	}*/
/*	if (file_fopen(&file,&efs.myFs,"HOSTDEMO.TXT",'w')==0)
	{ */
	    /* Write buffer to file */
//	    file_write(&file, 50, textFileBuffer);
	    /*close file and filesystem*/
/*	    file_fclose(&file);
	    fs_umount(&efs.myFs); 
	 }
	return 0;
} */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_mass_test_storage
* Returned Value : None
* Comments       :
*     Calls the UFI command set for testing
*END*--------------------------------------------------------------------*/

static void usb_host_mass_test_storage
   (
      void
   )
{ /* Body */
   USB_STATUS                                 status = USB_OK;
   uint_8                                     bLun = 0;
   uchar_ptr                                  buff_out, buff_in, p;
   INQUIRY_DATA_FORMAT                        inquiry;
   CAPACITY_LIST                              capacity_list;
   // clean warning MODE_SELECT_PARAMETER_LIST                 md_list;
   MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO read_capacity;
   MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO_PTR mCap;
   REQ_SENSE_DATA_FORMAT                      req_sense;
   // clean warning FORMAT_UNIT_PARAMETER_BLOCK                formatunit = { 0};
   
   int i;
   BU_U32 cap, sec;
   

   pCmd = (COMMAND_OBJECT_PTR) USB_Uncached_memalloc(sizeof(COMMAND_OBJECT_STRUCT));
   USB_memzero(pCmd,sizeof(COMMAND_OBJECT_STRUCT));
   
   mCap = (MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO_PTR)USB_Uncached_memalloc(sizeof(MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO));
   USB_memzero(mCap,sizeof(MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO));

   pCmd->LUN      = bLun;
   pCmd->CALL_PTR = (pointer)&mass_device.class_intf;
   pCmd->CALLBACK = callback_bulk_pipe;

   pCmd->CBW_PTR = (CBW_STRUCT_PTR) USB_Uncached_memalloc(sizeof(CBW_STRUCT));
   USB_memzero(pCmd->CBW_PTR,sizeof(CBW_STRUCT));

   pCmd->CSW_PTR = (CSW_STRUCT_PTR) USB_Uncached_memalloc(sizeof(CSW_STRUCT));
   USB_memzero(pCmd->CSW_PTR,sizeof(CSW_STRUCT));


   buff_in = (uchar_ptr)USB_Uncached_memalloc(0x200);
   
   DEBUGMSG(USB_DBG_LEVEL,("\n =============START OF A NEW SESSION==================\r\n"));

   /* Test the GET MAX LUN command */
   DEBUGMSG(USB_DBG_LEVEL,("\nTesting: GETMAXLUN Command"));
   bCallBack = FALSE;

   status = usb_class_mass_getmaxlun_bulkonly(
      (pointer)&mass_device.class_intf, &bLun,
      usb_host_mass_ctrl_callback);

   /* Wait till command comes back */
   while (!bCallBack) {;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */
   Host_inited = TRUE;
   /* Test the TEST UNIT READY command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: TEST UNIT READY Command \r\n"));

   bCallBack = FALSE;

   status =  usb_mass_ufi_test_unit_ready(pCmd);

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
       DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

  /* Test the REQUEST SENSE command */
  DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command \r\n"));
  bCallBack = FALSE;

  status = usb_mass_ufi_request_sense(pCmd, &req_sense,
      sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
       DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

   /* Test the INQUIRY command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: UFI_INQUIRY command \r\n"));

   bCallBack = FALSE;

   status = usb_mass_ufi_inquiry(pCmd, (uchar_ptr) &inquiry,
      sizeof(INQUIRY_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

   /* Test the REQUEST SENSE command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command \r\n"));
   bCallBack = FALSE;

   status = usb_mass_ufi_request_sense(pCmd, &req_sense,
      sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

   /* Test the READ FORMAT CAPACITY command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: READ FORMAT CAPACITY command \r\n"));
   
//   bCallBack = FALSE;
//   status = usb_mass_ufi_format_capacity(pCmd, (uchar_ptr)&capacity_list,
//   sizeof(CAPACITY_LIST));

//  while (!bCallBack){;}
//  if (!bStatus) {
//    DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//  } /* Endif */

//	p = (uchar_ptr)&capacity_list,
//	DEBUGMSG(TTC_DBG_LEVEL,("   format capacity = "));
//	for ( i=0; i<sizeof(CAPACITY_LIST); i++)
//		DEBUGMSG(USB_DBG_LEVEL,(" %02x ", *p++));


   /* Test the REQUEST SENSE command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command \r\n"));
//   fflush(stdout);
//  bCallBack = FALSE;

//  status = usb_mass_ufi_request_sense(pCmd, &req_sense,
//     sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
//  while (!bCallBack){;}
//  if (!bStatus) {
//     DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//  } /* Endif */

   /* Test the READ CAPACITY command */

   DEBUGMSG(USB_DBG_LEVEL,("Testing: READ CAPACITY command \r\n"));
   bCallBack = FALSE;

   status = usb_mass_ufi_read_capacity(pCmd, (uchar_ptr)mCap,
      sizeof(MASS_STORAGE_READ_CAPACITY_CMD_STRUCT_INFO));


   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

	p = (uchar *)mCap;
	for ( cap = 0,i=0; i<4; i++)
		cap = (cap << 8) | (BU_U32)(*p++);
	DEBUGMSG(USB_DBG_LEVEL,("\r\n  last block address = %d   \r\n  ", cap));

	for ( sec = 0,i=0; i<4; i++)
		sec = (sec << 8) | (BU_U32)(*p++);
	DEBUGMSG(USB_DBG_LEVEL,("  Sector Size = %d Bytes\r\n\r\n ", sec ));


	USB_memfree(mCap);

   /* Test the MODE SELECT command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: MODE SELECT command \r\n"));
//   fflush(stdout);
//   bCallBack = FALSE;

   //md_list. Fill parameters here
//   status = usb_mass_ufi_mode_select(pCmd, &md_list);

   /* Wait till command comes back */
//   while (!bCallBack){;}
//   if (!bStatus) {
//      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//   } /* Endif */

   /* Test the REQUEST SENSE command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command \r\n"));
   bCallBack = FALSE;

   status = usb_mass_ufi_request_sense(pCmd, &req_sense,
      sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

   /* Test the READ 10 command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: READ 10 command \n"));
   bCallBack = FALSE;

   status = usb_mass_ufi_read_10(pCmd, 0, buff_in, 512, 1);

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

	p = (uchar *)buff_in;
	DEBUGMSG(USB_DBG_LEVEL,(" Sector 0 = "));
	for ( i=0; i< 512; i++)
		DEBUGMSG(USB_DBG_LEVEL,(" %02x ", *p++));
	USB_memfree(buff_in);
   /* Test the MODE SENSE command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: MODE SENSE  command"));
//   fflush(stdout);
//   bCallBack = FALSE;

//   status = usb_mass_ufi_mode_sense(pCmd,
//      0, //PC
//      0x1c, //page code
//     buff_in,
//      0xC0);

   /* Wait till command comes back */
//   while (!bCallBack){;}
//   if (!bStatus) {
//      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//   } /* Endif */
   

   /* Test the PREVENT ALLOW command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: PREVENT ALLOW   command"));
//   fflush(stdout);
//   bCallBack = FALSE;

//   status = usb_mass_ufi_prevent_allow_medium_removal(
//      pCmd,
//     1 // prevent
//      );

   /* Wait till command comes back */
//   while (!bCallBack){;}
//   if (!bStatus) {
//      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//   } /* Endif */

   /* Test the REQUEST SENSE command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command"));
   bCallBack = FALSE;

   status = usb_mass_ufi_request_sense(pCmd, &req_sense,
      sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* End if */

    /*Test the VERIFY command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: VERIFY command"));
   bCallBack = FALSE;

   status = usb_mass_ufi_verify(
      pCmd,
      0x400, // block address
      1 //length to be verified
      );

   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* End if */

   /* Test the WRITE 10 command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: WRITE 10 command"));
   test_flag = TRUE;
   bCallBack = FALSE;
   buff_out = (uchar_ptr) USB_Uncached_memalloc(0x200);
   status = usb_mass_ufi_write_10(pCmd, 8, buff_out, 512, 1);

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

//   USB_memfree(buff_out);
   /* Test the REQUEST SENSE command */
   DEBUGMSG(USB_DBG_LEVEL,("Testing: REQUEST SENSE  command \r\n"));
   bCallBack = FALSE;

   status = usb_mass_ufi_request_sense(pCmd, &req_sense,
      sizeof(REQ_SENSE_DATA_FORMAT));

   /* Wait till command comes back */
   while (!bCallBack){;}
   if (!bStatus) {
      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
   } /* Endif */

//   DEBUGMSG(USB_DBG_LEVEL, ("Testing: file system \r\n"));
//   DEBUGMSG(USB_DBG_LEVEL, ("List all the files on USB Device \r\n"));
//   usb_host_mass_fs_test();
   /* Test the usb_mass_ufi_start_stop command */
//   DEBUGMSG(USB_DBG_LEVEL,("Testing: START STOP command \r\n"));
//   bCallBack = FALSE;

//   status = usb_mass_ufi_start_stop(pCmd, 0, 1);
 
   /* Wait till command comes back */
//   while (!bCallBack){;}
//   if (!bStatus) {
//      DEBUGMSG(USB_DBG_LEVEL,("...OK\n"));
//   } /* Endif */


   DEBUGMSG(USB_DBG_LEVEL,("\nTest done!"));
} /* Endbody */

/* EOF */

