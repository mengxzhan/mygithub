#ifndef __usb_hub_class_h__
#define __usb_hub_class_h__

#include "hostapi.h"

#define C_HUB_LOCAL_POWER		0
#define C_HUB_OVER_CURRENT		1
#define PORT_CONNECTION			0
#define PORT_ENABLE				1
#define PORT_SUSPEND			2
#define PORT_OVER_CURRENT		3
#define PORT_RESET				4
#define PORT_POWER				8
#define PORT_LOW_SPEED			9
#define PORT_HIGH_SPEED			10
//#define PORT_TEST				12
#define C_PORT_CONNECTION		16
#define C_PORT_ENABLE			17
#define C_PORT_SUSPEND			18
#define C_PORT_OVER_CURRENT		19
#define C_PORT_RESET			20
#define PORT_TEST				21
#define PORT_INDICATOR			22

#define  USB_DEVICE_IDLE                   (0)
#define  USB_DEVICE_ATTACHED               (1)
#define  USB_DEVICE_CONFIGURED             (2)
#define  USB_DEVICE_SET_INTERFACE_STARTED  (3)
#define  USB_DEVICE_INTERFACED             (4)
#define  USB_DEVICE_DETACHED               (5)
#define  USB_DEVICE_OTHER                  (6)

typedef struct device_struct
{
   uint_32                          dev_state;  /* Attach/detach state */
   PIPE_BUNDLE_STRUCT_PTR           pbs;        /* Device & pipes */
   _usb_device_instance_handle      dev_handle;
   _usb_interface_descriptor_handle intf_handle;
   CLASS_CALL_STRUCT                class_intf; /* Class-specific info */

} DEVICE_STRUCT,  _PTR_ DEVICE_STRUCT_PTR;

typedef struct usb_hub_descriptor
{
   uint_8   bLength;          /* Descriptor size in bytes = 9 */
   uint_8   bDescriptorType;  /* CONFIGURATION type = 2 or 7 */
   uint_8	bNbrPorts;		  /* Number of ports */
   uint_8   wHubCharacteristics[2];  /* Hub Characteristics */
   uint_8   bPwrOn2PwrGood;   /* time from power-on sequence to good */
   uint_8   bHubContrCurrent; /* Max current of the Hub in mA */ 
   uint_8   bDeviceRemovable; /* Indicating device is removable - up to 255 ports (32 bytes) */
   uint_8	bPortPwrCtrlMask[1]; /* For compatibility with USB 1.0 */
} eHUB_DESCRIPTOR, _PTR_ HUB_DESCRIPTOR_PTR;

/*
** USB Hub Class Interface structure. This structure will be
** passed to all commands to this class driver. The structure holds all information
** pertaining to an interface on vendor define device. This allows the class driver
** to know which interface the command is directed for.
*/

typedef struct _Usb_Hub_Intf_Struct {
   struct _Usb_Hub_Intf_Struct _PTR_  NEXT;              /* next storage device */
   struct _Usb_Hub_Intf_Struct _PTR_  ANCHOR;            /* list struct in common mem */
   _usb_device_instance_handle         DEV_INST;          /* device instance*/
   _usb_interface_descriptor_handle    INTF_DESC;         /* interface descriptor pointer*/
   _usb_host_handle                    HOST_HANDLE;
   uint_32                             KEY_CODE;
   _usb_pipe_handle                    CONTROL_PIPE;      /* control pipe handle*/
   _usb_pipe_handle					   INT_IN_PIPE; 	  /* data pipe handles - 1 int in ep for hub */
   uint_8                              INTERFACE_NUM;     /* interface number*/
   uint_8                              ALTERNATE_SETTING; /* Alternate setting*/
   uint_8							   hub_no;
   uint_8							   bReserve2;
   eHUB_DESCRIPTOR					   hub_desc;
} USB_HUB_CLASS_INTF_STRUCT, _PTR_ USB_HUB_CLASS_INTF_STRUCT_PTR;

typedef struct _Usb_Hub_Callback_Parm {
   USB_HUB_CLASS_INTF_STRUCT_PTR		intf_ptr;
   uchar_ptr							buffer;			// data buffer
   uint_32 								ulDataSize;
   uint_32								ulCount;		// transfered byte count
   uint_8								nEPIdx;
   uint_8								nPipeType;
} USB_HUB_CALLBACK_PARM, _PTR_ USB_HUB_CALLBACK_PARM_PTR;

#ifdef __cplusplus
extern "C" {
#endif

extern USB_HUB_CLASS_INTF_STRUCT_PTR GetHubClassIntf(CLASS_CALL_STRUCT_PTR  ccs_ptr);
extern void usb_class_hub_init(PIPE_BUNDLE_STRUCT_PTR, CLASS_CALL_STRUCT_PTR);
extern USB_STATUS usb_class_hub_setportfeature(_usb_device_instance_handle, uint_16, uint_16);

void usb_host_hub_device_event
    (
        _usb_device_instance_handle,
        _usb_interface_descriptor_handle,
        uint_32
     );

void usb_host_hub_ctrl_callback(

                  _usb_pipe_handle,
                  pointer,
                  uchar_ptr,
                  uint_32,
                  uint_32
                  );

void usb_host_hub_data_callback(
                  _usb_pipe_handle,
                  pointer,
                  uchar_ptr,
                  uint_32,
                  uint_32
                  );

void _usb_display_active_async_list
   (
      _usb_host_handle
   );

unsigned char usb_host_hub_test(void);

#ifdef __cplusplus
}
#endif

#endif

