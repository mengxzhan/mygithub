#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "usbprv_host.h"
#include "BU_Interrupt.h"
/**************************************************************************
Local header files for this application
**************************************************************************/
#include "usb_hub_class.h"

//extern sUsb2CI_CONFIG gsUsb2CIConfig;
USB_HUB_CALLBACK_PARM	hub_callback_parm;
extern UINT uLogLevel;
/* Linked list of instances of hub interfaces */
static USB_HUB_CLASS_INTF_STRUCT_PTR hub_anchor = NULL;

////////////////////////////////////////////////////////////////////////////

/**************************************************************************
   Global variables
**************************************************************************/
volatile DEVICE_STRUCT       hub_device = { 0 };   /* vendor defined device struct */
volatile uint_32 bHubCallBack = FALSE;
volatile USB_STATUS bHubStatus = USB_OK;



// Get a point to vendor defined class interface
USB_HUB_CLASS_INTF_STRUCT_PTR GetHubClassIntf(CLASS_CALL_STRUCT_PTR  ccs_ptr)
{
	USB_STATUS					  status = USB_OK;
	USB_HUB_CLASS_INTF_STRUCT_PTR  intf_ptr = NULL;

	if (usb_host_class_intf_validate(ccs_ptr)) {
      intf_ptr = (USB_HUB_CLASS_INTF_STRUCT_PTR) ccs_ptr->class_intf_handle;
      status = usb_hostdev_validate(intf_ptr->DEV_INST);
	} /* Endif */

	if (status == USB_OK)
		return intf_ptr;
	else
		return NULL;
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hub_ctrl_calback
* Returned Value : None
* Comments       :
*     called on completion of a control-pipe transaction.
*
*END*--------------------------------------------------------------------*/

void usb_host_hub_ctrl_callback
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

   bHubStatus = status;
   bHubCallBack = TRUE;

} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hub_data_callback
* Returned Value : None
* Comments       :
*     This routine is called when an isoc data transfer finished.
*		-- Need more work for multiple data endpoint transfer
*
*END*--------------------------------------------------------------------*/

void usb_host_hub_data_callback
(
      /* [IN] Pipe on which CBW call was made */
      _usb_pipe_handle pipe_handle,

      /* [IN] Pointer to the class instance */
      pointer          user_parm,

      /* [IN] Pointer to data buffer */
      uchar_ptr        buffer,

      /* [IN] Length of data transfered */
      uint_32          length_data_transfered,

      /* [IN] Status of the transfer */
      uint_32          status
   )
{ /* Body */
   USB_HUB_CALLBACK_PARM_PTR   phub_parm;

   //USB_LOG_INFO("%s: Enter status = 0x%x, buffer = 0x%x, length_data_transfered = %d", 
   //   __FUNCTION__, status, buffer, length_data_transfered);

   if (user_parm) {
      /* Get the pointer to the pending request */
      phub_parm = (USB_HUB_CALLBACK_PARM_PTR)user_parm;
	  //if (pvd_parm->nPipeType == USB_ISOCHRONOUS_PIPE)
	  //{
	  //   LogMsg(0,"+Ct=%0x%x,buf=0x%x,len=0x%x,pvd_buf=0x%x, EP%d", pvd_parm->ulCount, buffer, length_data_transfered, pvd_parm->buffer, pvd_parm->nEPIdx);
	  //}
	  bHubCallBack |= 1 << phub_parm->nEPIdx;
//	  HexDisplay(bHubCallBack);
   } /* Endif */
   bHubStatus = status;
   //LogMsg(0, "EP%d: bVDCallBack = 0x%x", pvd_parm->nEPIdx, bVDCallBack);   
   
} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_class_hub_getportstatus
* Returned Value : ERROR STATUS of the command
* Comments       :
*     Get status for a hub port
*END*--------------------------------------------------------------------*/
USB_STATUS usb_class_hub_getportstatus
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,
	  /* hub port */
      uint_8		                bPort,
	  /* port status */
	  uint_8_ptr					pStatus
   )
{ /* Body */
   USB_STATUS                       status = USBERR_NO_INTERFACE;
   USB_SETUP                        request;

   DEBUGMSG(USB_DBG_LEVEL,("%s", __FUNCTION__));
   
   bHubCallBack = 0;
   /* Get the number of logical units on the device */
   request.BMREQUESTTYPE = REQ_TYPE_CLASS | REQ_TYPE_OTHER | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_STATUS;
   htou16(request.WVALUE, 0);
   htou16(request.WINDEX, (uint_16)bPort);
   htou16(request.WLENGTH, 4);
   //USB_LOG_INFO("%x, %x, %x %x, %x %x, %x %x", request.BMREQUESTTYPE, request.BREQUEST, 
   //		request.WVALUE[0], request.WVALUE[1], request.WINDEX[0], request.WINDEX[1], request.WLENGTH[0], request.WLENGTH[1]);

   /* Send request */
   status = _usb_hostdev_cntrl_request (dev_handle,
      &request, pStatus, usb_host_hub_ctrl_callback, NULL);
   while (!bHubCallBack) {;}
   status = bHubStatus; 

   if (status == USB_OK)
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s: SUCCESSFUL \r\n", __FUNCTION__));
   }
   else
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s Exit with status = 0x%x \r\n", __FUNCTION__, (unsigned int)status));
   }   
   return status;

} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_class_hub_setportfeature
* Returned Value : ERROR STATUS of the command
* Comments       :
*     Clear feature for a hub port
*END*--------------------------------------------------------------------*/
USB_STATUS usb_class_hub_clearportfeature
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,
      /* feature selection */
      uint_16           			feature,
	  /* Select & Port */
      uint_16                		wIndex
   )
{ /* Body */
   USB_STATUS                       status = USBERR_NO_INTERFACE;
   USB_SETUP                        request;

   DEBUGMSG(USB_DBG_LEVEL,("%s", __FUNCTION__));
   
   bHubCallBack = 0;
   /* Get the number of logical units on the device */
   request.BMREQUESTTYPE = REQ_TYPE_CLASS | REQ_TYPE_OTHER | REQ_TYPE_OUT;
   request.BREQUEST = REQ_CLEAR_FEATURE;
   htou16(request.WVALUE, feature);
   htou16(request.WINDEX, wIndex);
   htou16(request.WLENGTH, 0);
   //USB_LOG_INFO("%x, %x, %x %x, %x %x, %x %x", request.BMREQUESTTYPE, request.BREQUEST, 
   //		request.WVALUE[0], request.WVALUE[1], request.WINDEX[0], request.WINDEX[1], request.WLENGTH[0], request.WLENGTH[1]);

   /* Send request */
   status = _usb_hostdev_cntrl_request (dev_handle,
      &request, NULL, usb_host_hub_ctrl_callback, NULL);
   while (!bHubCallBack) {;}
   status = bHubStatus; 

   if (status == USB_OK)
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s: SUCCESSFUL \r\n", __FUNCTION__));
   }
   else
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s Exit with status = 0x%x \r\n", __FUNCTION__, (unsigned int)status));
   }   
   return status;

} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_class_hub_setportfeature
* Returned Value : ERROR STATUS of the command
* Comments       :
*     Set feature for a hub port
*END*--------------------------------------------------------------------*/
USB_STATUS usb_class_hub_setportfeature
   (
      /* usb device */
      _usb_device_instance_handle   dev_handle,
      /* feature selection */
      uint_16           			feature,
	  /* Select & Port */
      uint_16                		wIndex
   )
{ /* Body */
   USB_STATUS                       status = USBERR_NO_INTERFACE;
   USB_SETUP                        request;

   DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));
   
   bHubCallBack = 0;
   /* Get the number of logical units on the device */
   request.BMREQUESTTYPE = REQ_TYPE_CLASS | REQ_TYPE_OTHER | REQ_TYPE_OUT;
   request.BREQUEST = REQ_SET_FEATURE;
   htou16(request.WVALUE, feature);
   htou16(request.WINDEX, wIndex);
   htou16(request.WLENGTH, 0);
   //USB_LOG_INFO("%x, %x, %x %x, %x %x, %x %x", request.BMREQUESTTYPE, request.BREQUEST, 
   //		request.WVALUE[0], request.WVALUE[1], request.WINDEX[0], request.WINDEX[1], request.WLENGTH[0], request.WLENGTH[1]);

   /* Send request */
   status = _usb_hostdev_cntrl_request (dev_handle,
      &request, NULL, usb_host_hub_ctrl_callback, NULL);
   while (!bHubCallBack) {;}
   status = bHubStatus; 

   if (status == USB_OK)
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s: SUCCESSFUL \r\n", __FUNCTION__));
   }
   else
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s Exit with status = 0x%x \r\n", __FUNCTION__, (unsigned int)status));
   }   
   return status;

} /* Endbody */

USB_STATUS usb_class_hub_setportfeature_port_reset
   (
      _usb_device_instance_handle   dev_handle,
	  uint_8		                bPort
   )
{
	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	return (usb_class_hub_setportfeature(dev_handle, PORT_RESET, (uint_16)bPort));
}

USB_STATUS usb_class_hub_setportfeature_port_power
	(
		_usb_device_instance_handle   dev_handle,
		uint_8		                  bPort
	)
{
	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	return (usb_class_hub_setportfeature(dev_handle, PORT_POWER, (uint_16)bPort));
}

USB_STATUS usb_class_hub_clearportfeature_port_connection_change
	(
		_usb_device_instance_handle   dev_handle,
		uint_8		                  bPort
	)
{
	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	return (usb_class_hub_clearportfeature(dev_handle, C_PORT_CONNECTION, (uint_16)bPort));
}

USB_STATUS usb_class_hub_clearportfeature_port_reset_change
	(
		_usb_device_instance_handle   dev_handle,
		uint_8		                  bPort
	)
{
	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	return (usb_class_hub_clearportfeature(dev_handle, C_PORT_RESET, (uint_16)bPort));
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_class_hub_getdescriptor
* Returned Value : ERROR STATUS of the command
* Comments       :
*     This is a class specific command. See the documentation of the USB
*     mass storage specifictaion to learn how this command works. This command
*     is used the get the Number of Logical Units on the device. Caller will
*     use the LUN number to direct the commands (as a part of CBW)
*END*--------------------------------------------------------------------*/

USB_STATUS usb_class_hub_getdescriptor
   (
      _usb_device_instance_handle   dev_handle,
      uint_8_ptr                 	pDesc
   )
{ /* Body */
   USB_STATUS                       status = USB_OK;
   USB_SETUP                        request;

   DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

   bHubCallBack = 0;
   /* Get the hub descriptor for this hub device */
   request.BMREQUESTTYPE = REQ_TYPE_CLASS | REQ_TYPE_DEVICE | REQ_TYPE_IN;
   request.BREQUEST = REQ_GET_DESCRIPTOR;
   htou16(request.WVALUE, 0);	// Hub descriptor type; index 0
   htou16(request.WINDEX, 0);	// 0 for Get Hub Descriptor
   htou16(request.WLENGTH, 0x47);

   /* Send request */
   status = _usb_hostdev_cntrl_request (dev_handle,
      &request, (uchar_ptr)pDesc, usb_host_hub_ctrl_callback, NULL);
   
   while (!bHubCallBack) {;}
   status = bHubStatus;

   if (status == USB_OK)
   {
      DEBUGMSG(USB_DBG_LEVEL,("%s: SUCCESSFUL \r\n", __FUNCTION__));
   }
   else
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s Exit with status = 0x%x \r\n", __FUNCTION__, (unsigned int)status));
   }
   
   return status;

} /* Endbody */

#if 0
void _usb_display_active_async_list
   (
      /* [IN] the USB Host state structure */
      _usb_host_handle                 handle
   )
{ /* Body */
   USB_HOST_STATE_STRUCT_PTR                    usb_host_ptr;
   ACTIVE_QH_MGMT_STRUCT_PTR                    active_list_member_ptr;
//   ACTIVE_QH_MGMT_STRUCT_PTR                    prev_ptr = NULL, next_ptr = NULL;
   EHCI_QH_STRUCT_PTR                           QH_ptr;

	usb_host_ptr = (USB_HOST_STATE_STRUCT_PTR)handle;

	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	/* Get the head of the active queue head */
	active_list_member_ptr = usb_host_ptr->ACTIVE_ASYNC_LIST_PTR;

	if (active_list_member_ptr)
	{
		while (active_list_member_ptr) 
		{
			QH_ptr = (EHCI_QH_STRUCT_PTR)((uint_32)active_list_member_ptr->QH_PTR & EHCI_QH_HORIZ_PHY_ADDRESS_MASK);
			USB_LOG_INFO("ACTIVE_QH_MGMT_STRUCT_PTR=0x%x, QH_PTR=0x%x, FIRST_QTD_PTR=0x%x, NEXT_ACTIVE_QH_MGMT_STRUCT_PTR=0x%x",
				 (unsigned int)active_list_member_ptr, (unsigned int)active_list_member_ptr->QH_PTR, 
				 (unsigned int)active_list_member_ptr->FIRST_QTD_PTR, (unsigned int)active_list_member_ptr->NEXT_ACTIVE_QH_MGMT_STRUCT_PTR);
			USB_LOG_INFO("HORIZ_LINK_PTR=0x%x, EP_CAPAB_CHARAC1=0x%x, EP_CAPAB_CHARAC2=0x%x",
				(unsigned int)QH_ptr->HORIZ_LINK_PTR, (unsigned int)QH_ptr->EP_CAPAB_CHARAC1, (unsigned int)QH_ptr->EP_CAPAB_CHARAC2);
	        active_list_member_ptr = 
	            active_list_member_ptr->NEXT_ACTIVE_QH_MGMT_STRUCT_PTR;
	    }
	}
	else
	{
		DEBUGMSG(USB_DBG_LEVEL,("No active async list exists. \r\n"));
	}
}
#endif

USB_STATUS usb_class_hub_data_transfer
   (
      /* [IN] Interface handle */
      USB_HUB_CLASS_INTF_STRUCT_PTR   intf_ptr, 
	  uint_8_ptr 	pBuf,  
	  tr_callback callback,
	  USB_HUB_CALLBACK_PARM_PTR		callback_parm
   )
{ /* Body */
	USB_STATUS                    status = USB_OK;
	TR_INIT_PARAM_STRUCT          tr_init;
	PIPE_DESCRIPTOR_STRUCT_PTR    pipe_descr_ptr;
   
	DEBUGMSG(USB_DBG_LEVEL,("%s \r\n", __FUNCTION__));

	pipe_descr_ptr = (PIPE_DESCRIPTOR_STRUCT_PTR)intf_ptr->INT_IN_PIPE;
	callback_parm->intf_ptr = intf_ptr;
	callback_parm->buffer = 0;
	callback_parm->ulDataSize = pipe_descr_ptr->MAX_PACKET_SIZE;
	callback_parm->ulCount = 0;
	callback_parm->nEPIdx = pipe_descr_ptr->ENDPOINT_NUMBER;
	callback_parm->nPipeType = pipe_descr_ptr->PIPETYPE; 

	usb_hostdev_tr_init(&tr_init, callback, (pointer)callback_parm);

	if (pipe_descr_ptr->DIRECTION == USB_RECV)
	{
		tr_init.RX_BUFFER = pBuf;
		tr_init.RX_LENGTH = pipe_descr_ptr->MAX_PACKET_SIZE;
		//LogMsg(0, "tr_init.RX_BUFFER = 0x%x, tr_init.RX_LENGTH = 0x%x, pipe_descr_ptr = 0x%x", 
		//	pBuf, ulDataSize, pipe_descr_ptr);
		status = _usb_host_recv_data(intf_ptr->HOST_HANDLE,
            pipe_descr_ptr, &tr_init);
	}
	else
	{
		DEBUGMSG(USB_DBG_LEVEL,("Hub interrupt endpoint has wrong direction - interrupt out \r\n"));
		status = USBERR_BAD_STATUS;
	}
    
	return status;
}

USB_STATUS usb_class_hub_enum_downstream_device
   (
      /* [IN] Interface handle */
      USB_HUB_CLASS_INTF_STRUCT_PTR   intf_ptr, 
	  uint_8 						  bPort,
	  uchar_ptr                       buf_in  
   )
{ /* Body */
	USB_STATUS                    status = USB_OK;
	uint_16						  *pStatus, *pCStatus;
	uint_32						  *pPortStatus;
	uint_8						  bSpeed;
	int							  i;

	// Get port status
	usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
	sys_delay_us(1000);;
	usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
	pStatus = (uint_16_ptr)buf_in;
	pCStatus = (uint_16_ptr)(buf_in + 2);
	DEBUGMSG(USB_DBG_LEVEL,("Port #%d status = 0x%x, change status = 0x%x \r\n", (unsigned int)bPort, (unsigned int)*pStatus, (unsigned int)*pCStatus));

	// The none build-in hub need to reset a port first to get Current Connect Status bit set
	usb_class_hub_setportfeature_port_reset(intf_ptr->DEV_INST, bPort);

	sys_delay_us(1000);
	usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
	pStatus = (uint_16_ptr)buf_in;
	pCStatus = (uint_16_ptr)(buf_in + 2);
	DEBUGMSG(USB_DBG_LEVEL,("Port #%d status = 0x%x, change status = 0x%x \r\n", (unsigned int)bPort, (unsigned int)*pStatus, (unsigned int)*pCStatus));
	
	if (!(*pStatus & (1<<PORT_CONNECTION)))
		return USBERR_DEVICE_NOT_FOUND;

	// Enable period schedule
	usb_enable_period_schedule(intf_ptr->HOST_HANDLE);

	for (i=0; i<2; i++)
	{
		DEBUGMSG(USB_DBG_LEVEL,("%s, in for  \r\n", __FUNCTION__));
		// If there is a connection, try to reset and enumerate the downstream device
		usb_class_hub_setportfeature_port_reset(intf_ptr->DEV_INST, bPort);
		sys_delay_us(1000);

		// Receive data from interrupt pipe
		bHubCallBack = 0;
		status = usb_class_hub_data_transfer(intf_ptr, 
												buf_in, 
												usb_host_hub_data_callback, 
												&hub_callback_parm);
		DEBUGMSG(USB_DBG_LEVEL,("%s, after data transfer  \r\n", __FUNCTION__));
		/* Wait till command comes back */
		while (!bHubCallBack) {;}
		status = bHubStatus;

		usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
		pStatus = (uint_16_ptr)buf_in;
		pCStatus = (uint_16_ptr)(buf_in + 2);
		pPortStatus = (uint_32_ptr)buf_in;
		DEBUGMSG(USB_DBG_LEVEL,("Port #%d status = 0x%x, change status = 0x%x \r\n", (unsigned int)bPort, (unsigned int)*pStatus, (unsigned int)*pCStatus));

		if ((*pPortStatus & C_PORT_CONNECTION) || (*pPortStatus & C_PORT_RESET))
		{
			// If port status is still changing, wait until it stable
			do
			{
				if (*pPortStatus & C_PORT_RESET)
				{
					sys_delay_us(1000);
					status = usb_class_hub_clearportfeature_port_reset_change(intf_ptr->DEV_INST, bPort);
				}
				if (*pPortStatus & C_PORT_CONNECTION)
				{
					sys_delay_us(1000);
					status = usb_class_hub_clearportfeature_port_connection_change(intf_ptr->DEV_INST, bPort);
				}
				status = usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
				sys_delay_us(1000);
				status = usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
				pStatus = (uint_16_ptr)buf_in;
				pCStatus = (uint_16_ptr)(buf_in + 2);
				pPortStatus = (uint_32_ptr)buf_in;
				DEBUGMSG(USB_DBG_LEVEL, ("4 byte port #%d status = 0x%x \r\n", (unsigned int)bPort, (unsigned int)*pPortStatus));
			}
			while ((*pPortStatus & C_PORT_CONNECTION) || (*pPortStatus & C_PORT_RESET));
		}
	}

	if (*pPortStatus & (1<<PORT_CONNECTION || 1<<PORT_ENABLE))
	{
		DEBUGMSG(USB_DBG_LEVEL,("%s, port is stable  \r\n", __FUNCTION__));
				if (*pPortStatus & PORT_HIGH_SPEED)
					bSpeed = USB_SPEED_HIGH;
				else if (*pPortStatus & PORT_LOW_SPEED)
					bSpeed = USB_SPEED_LOW;
				else
					bSpeed = USB_SPEED_FULL;

				// Create a new device instance for the USB device connected to the hub
//				usb_dev_list_attach_device(intf_ptr->HOST_HANDLE, bSpeed, intf_ptr->hub_no, bPort);
	}

	DEBUGMSG(USB_DBG_LEVEL,("%s exit with status=0x%x", __FUNCTION__, (unsigned int)status));
		 
	return status;
}

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hub_test
* Returned Value : None
* Comments       :
*     Test on a vendor defined USB device
*END*--------------------------------------------------------------------*/
unsigned char usb_host_hub_test
   (
      void
   )
{ /* Body */
	USB_STATUS                    status = USB_OK;
	uint_16						  dev_Vender, dev_Product, dev_Status, *pStatus, *pCStatus;
	uint_8						  bPort;
	uchar_ptr                     buf_in;
//	int							  i;
	DEV_INSTANCE_PTR              dev_ptr;
	USB_HUB_CLASS_INTF_STRUCT_PTR intf_ptr;

	DEBUGMSG(USB_DBG_LEVEL,("%s", __FUNCTION__));

	intf_ptr = GetHubClassIntf((CLASS_CALL_STRUCT_PTR)&hub_device.class_intf);
	dev_ptr = (DEV_INSTANCE_PTR)intf_ptr->DEV_INST;
	dev_Vender = utoh16(dev_ptr->dev_descriptor.idVendor);
	dev_Product = utoh16(dev_ptr->dev_descriptor.idProduct);

	DEBUGMSG(USB_DBG_LEVEL,("Hub VID = 0x%04x; PID = 0x%04x \r\n", (unsigned int)dev_Vender, (unsigned int)dev_Product));

	_usb_host_register_ch9_callback(intf_ptr->DEV_INST,
		usb_host_hub_ctrl_callback, (pointer)intf_ptr);

	buf_in = (uchar_ptr) USB_Uncached_memalloc(0x100);//usbMalloc(gsUsb2CIConfig.gpHeap, 0x100, 4);

	// Get hub descriptor first
	status = usb_class_hub_getdescriptor(intf_ptr->DEV_INST, buf_in);

	if (status == USB_OK) 
	{
		memcpy(&intf_ptr->hub_desc, buf_in, sizeof(eHUB_DESCRIPTOR));
		DEBUGMSG(USB_DBG_LEVEL,("The hub has %d ports \r\n", (unsigned int)intf_ptr->hub_desc.bNbrPorts));
	}

	// Get Device Status
	bHubCallBack = 0;
	ch9_get_status_ex(intf_ptr->DEV_INST, (uint_16_ptr)buf_in, usb_host_hub_ctrl_callback);
	/* Wait till command comes back */
	while (!bHubCallBack) {;}
	status = bHubStatus;

	dev_Status = *((uint_16_ptr)buf_in);
	DEBUGMSG(USB_DBG_LEVEL,("Dev Status = 0x%x \r\n", (unsigned int)dev_Status));

	// If the port is self powered, set port power now
	if (dev_Status & USB_SELF_POWERED)
	{
		for (bPort=1; bPort<=intf_ptr->hub_desc.bNbrPorts; bPort++)
		{
			usb_class_hub_setportfeature_port_power(intf_ptr->DEV_INST, bPort);
		}
	}

	// Clear Port Connection Change bit
	for (bPort=1; bPort<=intf_ptr->hub_desc.bNbrPorts; bPort++)
	{
		usb_class_hub_clearportfeature_port_connection_change(intf_ptr->DEV_INST, bPort);
	}

	// Get hub port status
	for (bPort=1; bPort<=intf_ptr->hub_desc.bNbrPorts; bPort++)
	{
		usb_class_hub_getportstatus(intf_ptr->DEV_INST, bPort, buf_in);
		pStatus = (uint_16_ptr)buf_in;
		pCStatus = (uint_16_ptr)(buf_in + 2);
		DEBUGMSG(USB_DBG_LEVEL,("Port #%d status = 0x%x, change status = 0x%x \r\n", (unsigned int)bPort, (unsigned int)*pStatus, (unsigned int)*pCStatus));
	}

//	if (gsUsb2CIConfig.gbHsicPort)
//	{
		// For HSIC, we only have port 2 available. Enumerate its downstream device
//		do
//		{
//			status = usb_class_hub_enum_downstream_device(intf_ptr, 2, buf_in);
//		} while (status == USBERR_DEVICE_NOT_FOUND);
//	}
//	else
/*	{
		do
		{
			for (bPort=1; bPort<=intf_ptr->hub_desc.bNbrPorts; bPort++)
			{
				status = usb_class_hub_enum_downstream_device(intf_ptr, bPort, buf_in);
				if (status != USBERR_DEVICE_NOT_FOUND)
					break;
			}
		} while (status == USBERR_DEVICE_NOT_FOUND);
	}
*/
	USB_memfree(buf_in);

	return 1;
} /* Endbody */

unsigned char hub_test(void)
{ /* Body */
	unsigned char ucRet;
	USB_STATUS           status = USB_OK;

	hub_device.dev_state = 0;
	hub_device.pbs = NULL;			/* Device & pipes */      
	hub_device.dev_handle = NULL; 
	hub_device.intf_handle = NULL;
	//   CLASS_CALL_STRUCT
	hub_device.class_intf.class_intf_handle = NULL; /* Class-specific info */
	hub_device.class_intf.code_key = 0; /* Class-specific info */

	bHubCallBack = FALSE;
	bHubStatus   = USB_OK;

	//Attach_Notification = FALSE;
	
   /*----------------------------------------------------**
   ** Infinite loop, waiting for events requiring action **
   **----------------------------------------------------*/
   for ( ; ; ) {
      switch ( hub_device.dev_state ) {
         case USB_DEVICE_IDLE:
            break ;
         case USB_DEVICE_ATTACHED:
            hub_device.dev_state = USB_DEVICE_SET_INTERFACE_STARTED;
            status = _usb_hostdev_select_interface(hub_device.dev_handle,
                 hub_device.intf_handle, (pointer)&hub_device.class_intf);
            break ;
         case USB_DEVICE_SET_INTERFACE_STARTED:
            break;
         case USB_DEVICE_INTERFACED:
            DEBUGMSG(USB_DBG_LEVEL, ("\nUSB Focus Device Interfaced, Enter Focus test\r\n"));
            ucRet = usb_host_hub_test();
            hub_device.dev_state = USB_DEVICE_OTHER;
//            break ;
			return ucRet;
			
         case USB_DEVICE_DETACHED:
            DEBUGMSG(USB_DBG_LEVEL, ("USB Hub Device Detached\n"));
            hub_device.dev_state = USB_DEVICE_IDLE;
            break;
         case USB_DEVICE_OTHER:
            break ;
         default:
		  DEBUGMSG(USB_DBG_LEVEL,("%s: Unknown USB Hub Device State = %d\n",
               __FUNCTION__, (int)hub_device.dev_state));
            break ;
      } /* Endswitch */
   } /* Endfor */

   return TRUE;

} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_host_hub_device_event
* Returned Value : None
* Comments       :
*     called when mass storage device has been attached, detached, etc.
*END*--------------------------------------------------------------------*/
void usb_host_hub_device_event
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

   DEBUGMSG(USB_DBG_LEVEL,("%s, event_code = %d", __FUNCTION__, (int)event_code));

   switch (event_code) {
      case USB_CONFIG_EVENT:
         /* Drop through into attach, same processing */
      case USB_ATTACH_EVENT:
         if (hub_device.dev_state == USB_DEVICE_IDLE) {
            hub_device.dev_handle = dev_handle;
            hub_device.intf_handle = intf_handle;
            hub_device.dev_state = USB_DEVICE_ATTACHED;
            DEBUGMSG(USB_DBG_LEVEL,("USB Hub Device Attached \r\n"));
         } else {
        	 DEBUGMSG(USB_DBG_LEVEL,("USB Hub Device Already Attached \r\n"));
         } /* EndIf */
         break;
      case USB_INTF_EVENT:
         hub_device.dev_state = USB_DEVICE_INTERFACED;
         break;
      case USB_DETACH_EVENT:
         if (hub_device.dev_state == USB_DEVICE_INTERFACED) {
            hub_device.dev_handle = NULL;
            hub_device.intf_handle = NULL;
            hub_device.dev_state = USB_DEVICE_DETACHED;
         } else {
        	 DEBUGMSG(USB_DBG_LEVEL,("USB Hub Device Not Attached \r\n"));
         } /* EndIf */
         break;
      default:
         hub_device.dev_state = USB_DEVICE_IDLE;
         break;
   } /* EndSwitch */

} /* Endbody */

/*FUNCTION*----------------------------------------------------------------
*
* Function Name  : usb_class_hub_init
* Returned Value : None
* Comments       :
*     This function initializes a vendor defined USB class driver
*
*END*--------------------------------------------------------------------*/
void usb_class_hub_init
   (
      /* [IN] structure with USB pipe information on the interface */
      PIPE_BUNDLE_STRUCT_PTR      pbs_ptr,

      /* [IN] printer call struct pointer */
      CLASS_CALL_STRUCT_PTR       ccs_ptr
   )
{ /* Body */
   USB_HUB_CLASS_INTF_STRUCT_PTR   intf_ptr = 
      (USB_HUB_CLASS_INTF_STRUCT_PTR)(ccs_ptr->class_intf_handle);
   DEV_INSTANCE_PTR                 dev_ptr;
   //DESCRIPTOR_UNION           		desc;
   //uint_8							m, n;

   DEBUGMSG(USB_DBG_LEVEL,("%s", __FUNCTION__));

   hub_anchor = NULL;
	
   /* Pointer validity-checking, clear memory, init header */
   if (usb_host_class_intf_init(pbs_ptr, intf_ptr, &hub_anchor) != USB_OK) 
   {
	   DEBUGMSG(USB_DBG_LEVEL,("%s: call usb_host_class_intf_init() failed. \r\n", __FUNCTION__));
      return;
   }

   USB_lock();
   ccs_ptr->code_key = 0;
   ccs_ptr->code_key = usb_host_class_intf_validate( ccs_ptr );
   if (ccs_ptr->code_key == 0) 
   {
	   DEBUGMSG(USB_DBG_LEVEL,("usb_host_class_intf_validate failed \r\n"));
      goto Bad_Exit;
   }
   //DEBUGMSG(0, "%s: ccs_ptr->code_key = %d", __FUNCTION__, (unsigned int)ccs_ptr->code_key);

   /* Start filling up the members of interface handle */
   dev_ptr = (DEV_INSTANCE_PTR)intf_ptr->DEV_INST ;

   intf_ptr->DEV_INST         = pbs_ptr->dev_handle;
   intf_ptr->INTF_DESC        = pbs_ptr->intf_handle;
   intf_ptr->HOST_HANDLE      = ((DEV_INSTANCE_PTR)pbs_ptr->dev_handle)->host;
   intf_ptr->INTERFACE_NUM =
      ((INTERFACE_DESCRIPTOR_PTR)intf_ptr->INTF_DESC)->bInterfaceNumber;
   intf_ptr->hub_no = 1;		// We only support one leve hub for now

   intf_ptr->CONTROL_PIPE = usb_hostdev_get_pipe_handle(pbs_ptr,
      USB_CONTROL_PIPE, 0);
   intf_ptr->INT_IN_PIPE = usb_hostdev_get_pipe_handle(pbs_ptr,
      USB_INTERRUPT_PIPE, USB_RECV);
      
   if (intf_ptr->CONTROL_PIPE &&
      intf_ptr->INT_IN_PIPE)  
   {
      /* Initialize the queue for storing the local copy of interface handles */
      // usb_class_mass_q_init(intf_ptr);
      USB_unlock();

      DEBUGMSG(USB_DBG_LEVEL,("%s, Exit SUCCESSFUL \r\n", __FUNCTION__));
      return;
   } /* Endif */ 
   
   USB_unlock();

   DEBUGMSG(USB_DBG_LEVEL,("%s, Exit SUCCESSFUL \r\n", __FUNCTION__));
   return;

Bad_Exit:

   DEBUGMSG(USB_DBG_LEVEL,("%s, bad exit \r\n", __FUNCTION__));
   USB_memzero(intf_ptr, sizeof(USB_HUB_CLASS_INTF_STRUCT_PTR));
   ccs_ptr->class_intf_handle = NULL;
   ccs_ptr->code_key = 0;
   USB_unlock();
} /* Endbody */

/* EOF */
