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
***   This file contains the VUSB functions specific to ARC processor.
***                                                               
**************************************************************************
**END*********************************************************/
#include "usbh_arm.h"
#include "stdlib.h"

#include "..\include\USB.h"
#include "USB_SPH.h"

static uint_8 disable_count = 0;
boolean  IN_ISR = FALSE;
/*#define debug 1*/
#ifdef debug
uint_32 test[100];
#define max 500
uint_32 store_flush[max],i=0;
#endif
#include "BU_SWconf.h"
#include "Mv_malloc.h"
/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : USB_int_install_isr
*  Returned Value : None
*  Comments       :
*        Installs the USB interrupt service routine
*
*END*-----------------------------------------------------------------*/

//#if 0 // fl: replace USB_int_install_isr with HM2 ISR_connect()
void USB_int_install_isr
   (
      /* [IN] vector number */
      uint_8 vector_number,

      /* [IN] interrupt service routine address */
      INTERRUPT_ROUTINE_KEYWORD void (_CODE_PTR_ isr_ptr)(void),
            
      /* [IN] parameter for ISR - unused for standalone version */
      pointer  handle
   )
{ /* Body */
#if ((MQX_CPU == 0xACA5) || (MQX_CPU == 0xACA52))  /* ARCtangent-A5 Core */

   volatile uint_16 _PTR_  loc_ptr;
   uint_32      loc, priority;

   loc     = (uint_32)isr_ptr;
   loc_ptr = (volatile uint_16 _PTR_)_lr(0x25);

   /* Offset into the vector table */   
   loc_ptr += (4 * vector_number);
   
   *loc_ptr++ = 0x2020; /* Hi word of JL */
   *loc_ptr++ = 0x0F80; /* Lo word of JL */
   *loc_ptr++ = loc >> 16;
   *loc_ptr   = (uint_16)loc;

   /* Get the value of the AUX_IRQ_LEVL auxiliary register */   
   priority = _lr(0x200);
  
   /* We want level 1 so zero out that IRQ bit position */
   priority &= ~((uint_32)1 << vector_number);
   
   /*  default to level one interrupt in level register */
   _sr((unsigned)(priority), (unsigned)(0x200));

   /* Invalidate ICache */
   _sr(0, 0x10);
  
#else
   uint_32_ptr                loc_ptr = (uint_32_ptr)0;

   /* Offset into the vector table */   
   loc_ptr += (2 * vector_number);
   
   *loc_ptr++ = ARC_JMP_TO_ADDR_INSTRN;   /* OpCode for Jump to addr */
   *loc_ptr++ = (uint_32)isr_ptr;         /* Address of the ISR */
#endif
 
} /* EndBody */
//#endif // fl: replace USB_int_install_isr with HM2 ISR_connect()


/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _bsp_get_usb_vector
*  Returned Value : interrupt vector number
*  Comments       :
*        Get the vector number for the specified device number
*END*-----------------------------------------------------------------*/

uint_8 _bsp_get_usb_vector
   (
      uint_8 device_number
   )
{ /* Body */
#ifdef __USB_HOST__
   if (device_number == 0) {
      return BSP_VUSB20_HOST_VECTOR0;
   } /* Endif */
#endif

#ifdef __USB_DEVICE__
   if (device_number == 0) {
      return BSP_VUSB20_DEVICE_VECTOR0;
   } /* Endif */
#endif

#ifdef __USB_OTG__
   if (device_number == 0) {
      return BSP_VUSB20_OTG_VECTOR0;
   } /* Endif */
#endif
	
	return 0;  // fl: clean warnings 
} /* EndBody */

/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _bsp_get_usb_base
*  Returned Value : Address of the VUSB register base
*  Comments       :
*        Get the USB register base address
*END*-----------------------------------------------------------------*/
static uint_32 usb_base[4] = { USB_BASE, USB_SPH0_BASE, USB_SPH1_BASE };

pointer _bsp_get_usb_base
   (
      uint_8 device_number
   )
{ /* Body */
   if (device_number == 0) {
      return (pointer)BSP_VUSB20_HOST_BASE_ADDRESS0;
   } /* Endif */
   
	if ( device_number < 4 )
		return (pointer)(usb_base[device_number] );
		
	return NULL;  // fl: clean warnings 
   
} /* EndBody */

/*FUNCTION*-------------------------------------------------------------
*
*  Function Name  : _bsp_get_usb_capability_register_base
*  Returned Value : Address of the VUSB1.1 capability register base
*  Comments       :
*        Get the USB capability register base address
*END*-----------------------------------------------------------------*/

pointer _bsp_get_usb_capability_register_base
   (
      /* [IN] the device number */
      uint_8 device_number
   )
{ /* Body */

	if (device_number == 0) {
		return ((pointer)BSP_VUSB20_HOST_BASE_ADDRESS0);
	} /* Endif */
	
	if ( device_number < 4 )
		return (pointer)(usb_base[device_number]+0x100);
	
	return NULL;  // fl: clean warnings 
   
} /* EndBody */

#ifndef __USB_OS_MQX__
void _disable_interrupts
   (
      void
   )
{ /* Body */
   
   if ((!disable_count) && (!IN_ISR)) {
      DISABLE_INTERRUPTS();
   } /* Endif */

   if (!IN_ISR) {
      disable_count++;
   } /* Endif */
   
} /* EndBody */

void _enable_interrupts
   (
      void
   )
{ /* Body */

   if (!IN_ISR) {
      disable_count--;
   } /* Endif */
   
   if ((!disable_count) && (!IN_ISR)) {
      ENABLE_INTERRUPTS();
   } /* Endif */
   
} /* EndBody */

#ifdef _DATA_CACHE_
/*****************************************************
DATA CACHE Routines                                                         
*****************************************************/

#define FAILURE 0
#define SUCCESS	!FAILURE
#define BITS_PER_ULONG 32

/*-------------------------------------------------------------*/
#define MASK( POS, WIDTH ) ( ( 0xFFFFFFFF << POS ) & ( 0xFFFFFFFF >> ( 32 - ( POS + WIDTH ) ) ) )
#define BIT_EXTRACT( DATA, POS, WIDTH )  ( (DATA & MASK( POS, WIDTH ) ) >> POS )
#define BIT_SET( DATA, POS, WIDTH, VALUE ) ( (DATA & ~(MASK( POS, WIDTH )) ) | ((VALUE & MASK( 0, WIDTH )) << POS) ) 

/*----------------------------------------------------------------------*/
#define DC_IVDC       0x47  /* Hex Address of Data Cache Invalidatae  */

#define DC_IVDC_IV    0     /* Invalidate command bit position */
#define DC_IVDC_IV_W  1     /* Invalidate command field width */

/*----------------------------------------------------------------------*/
#define DC_CTRL       0x48

#define DC_CTRL_DC    0
#define DC_CTRL_DC_W  1

#define DC_CTRL_EB    1
#define DC_CTRL_EB_W  1

#define DC_CTRL_SB    2
#define DC_CTRL_SB_W  1
#define DC_CTRL_SB_SUCCESS 1

#define DC_CTRL_RA    3
#define DC_CTRL_RA_W  2

#define DC_CTRL_AT    5
#define DC_CTRL_AT_W  1
#define DC_CTRL_AT_DIRECT 0
#define DC_CTRL_AT_CACHE  1

#define DC_CTRL_IM    6
#define DC_CTRL_IM_W  1

#define DC_CTRL_LM    7
#define DC_CTRL_LM_W  1

#define DC_CTRL_FS    8
#define DC_CTRL_FS_W  1
#define DC_CTRL_FS_IDLE 0
#define DC_CTRL_FS_BUSY 1

/*----------------------------------------------------------------------*/
#define DC_LDL        0x49

/*----------------------------------------------------------------------*/
#define DC_IVDL       0x4A

/*----------------------------------------------------------------------*/
#define DC_FLSH       0x4B

#define DC_FLSH_FL    0
#define DC_FLSH_FL_W  1

/*----------------------------------------------------------------------*/
#define DC_FLDL       0x4C

/*----------------------------------------------------------------------*/
#define DC_RAM_ADDR   0x58


/*----------------------------------------------------------------------*/
#define DC_TAG        0x59

#define DC_TAG_V      0
#define DC_TAG_V_W    1
#define DC_TAG_V_VALID  1

#define DC_TAG_L      1
#define DC_TAG_L_W    1
#define DC_TAG_L_LOCKED 1

#define DC_TAG_D      2
#define DC_TAG_D_W    1
#define DC_TAG_D_DIRTY  1



/*----------------------------------------------------------------------*/
#define DC_WP         0x5A

/*----------------------------------------------------------------------*/
#define DC_DATA       0x5B

/*----------------------------------------------------------------------*/
#define D_CACHE_BUILD           0x72

#define D_CACHE_BUILD_VERSION   0
#define D_CACHE_BUILD_VERSION_W 8

#define D_CACHE_BUILD_CONFIG    8
#define D_CACHE_BUILD_CONFIG_W  4

#define D_CACHE_CON_ONE_WAY_2   0x0
#define D_CACHE_CON_TWO_WAY_2   0x1
#define D_CACHE_CON_FOUR_WAY_2  0x2

#define D_CACHE_CON_ONE_WAY_3   0x3
#define D_CACHE_CON_TWO_WAY_3	0x4
#define D_CACHE_CON_FOUR_WAY_3  0x5



#define D_CACHE_BUILD_RAM_SZ    12
#define D_CACHE_BUILD_RAM_SZ_W  4
 

#define D_CACHE_BUILD_LINE_SZ   16
#define D_CACHE_BUILD_LINE_SZ_W 4



static unsigned int _dc_way_pos, _dc_way_width;     /* Bit position and Field width of WAY within the D-cache tag register.  */
static unsigned int _dc_index_pos, _dc_index_width; /* Bit positon and Field width of line INDEX within D-cache tag register.*/
static unsigned int    _dc_line_size; /* Data cache line size */


/*----------------------------------------------------------------------
 
              Call "_dcache_flush()" everytime you need to flush all lines
              in your data cache without the risk of unlocking lines in WAY-ZERO

              Call "_dcache_flush_mlines()" to force coherency with a specific memory
              region.

               Call "_dcache_flush_line()" to force coherency with a specific memory
              cache line.

              Call "_dcache_invalidate_()" everytime you need to flush all lines
              in your data cache without the risk of unlocking lines in WAY-ZERO

              Call "_dcache_invalidate_mlines()" to force coherency with a specific memory
              region.

              Call "_dcache_invalidate_line()" to force coherency with a specific memory cache line              region.

-----------------------------------------------------------------------------------*/


/*-----------------------------------------------------------------
   Run once to detect INDEX and WAY fields of D-cache tags
  Returns SUCCESS if all values are detected otherwise FAILURE.
-----------------------------------------------------------------*/
int _dcache_detect_config( void )
{
   
   unsigned int dc_config_value;
   unsigned int value;
   

   dc_config_value = _lr( D_CACHE_BUILD );
   
   /* Extract the RAM_SZ field from D_CACHE BCR */
   value = (BIT_EXTRACT( dc_config_value, D_CACHE_BUILD_RAM_SZ, D_CACHE_BUILD_RAM_SZ_W ) );
   _dc_way_pos = value + 9;  /* Cache memory size = exp2( RAM_SZ ) * 512 bytes
                             WAY field starts at log2( Cache memory size ) */

   /* Extract LINE_SZ from D_CACHE BCR */
   value = (BIT_EXTRACT( dc_config_value, D_CACHE_BUILD_LINE_SZ, D_CACHE_BUILD_LINE_SZ_W ) );
   _dc_index_pos = value + 4; /* Line Length =  exp2( LINE_SZ ) * 16 bytes
                              INDEX field starts at log2( Line Length ) */
   
   /* Extract the CONFIG field from D_CACHE BCR */
   value = (BIT_EXTRACT(dc_config_value, D_CACHE_BUILD_CONFIG, D_CACHE_BUILD_CONFIG_W ) );
   if(      value == D_CACHE_CON_FOUR_WAY_2 || value == D_CACHE_CON_FOUR_WAY_3 ) /* Any FOUR-WAY cache */
   { 
      _dc_way_pos -= 2;   /* Adjust WAY field positon since size of each way = Cache memory size / 4 */
      _dc_way_width = 2;  /* WAY field width = log2( 4-ways ) */
   } 
   else if( value == D_CACHE_CON_TWO_WAY_2  || value == D_CACHE_CON_TWO_WAY_3  ) /* Any TWO-WAY cache */
   { 
      _dc_way_pos -= 1;   /* Adjust WAY field positon since size of each way = Cache memory size / 2*/
      _dc_way_width = 1;  /* WAY field width = log2( 2-ways )*/
   } 
   else if( value == D_CACHE_CON_ONE_WAY_2 || value == D_CACHE_CON_ONE_WAY_3 ) /* Any ONE-WAY cache */
   { 
      _dc_way_width = 0;  /* WAY field width = log2( 1-way ) */
   } 
   else 
   { 
      return FAILURE;   /* D_cache CONFIG must be one of the previous six values,*/
                        /* otherwise return FAILURE.*/
   } 
   
   _dc_index_width = _dc_way_pos - _dc_index_pos;  /* Compute width of INDEX field */
  
   _dc_line_size = 1 << ( BIT_EXTRACT( _lr( D_CACHE_BUILD ), D_CACHE_BUILD_LINE_SZ, D_CACHE_BUILD_LINE_SZ_W) + 4); 



   return SUCCESS;   /* Return SUCCESS */
   
}
/*
---------------------------------------------------------------

  Purpose: Force coherency between the data cache and a memory region 


  void _dcache_flush(void)
 
  
-----------------------------------------------------------------
 Flush entire Data Cache using individual line flushes
-----------------------------------------------------------------
*/
void _dcache_flush( void )
{
   unsigned int old_ctrl,dc_control;
   unsigned int address, tag_addr, tag_value, flsh_addr;
   
   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings */
   
      
  dc_control = old_ctrl;      /* Set Data Cache control to Direct TAG Access Mode */
  dc_control =  BIT_SET(dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_DIRECT );
   _sr(dc_control, DC_CTRL ); 
     

   /* Walk through all Line Index in all ways to check for VALID and DIRTY bits */
   for( address = (0x01 << (_dc_index_width + _dc_way_width)); address != 0; address-- )
   { 
      /* Get the TAG value from the cache */
      tag_addr = BIT_SET( 0x0, _dc_index_pos, (_dc_index_width + _dc_way_width), address );
      _sr( tag_addr, DC_RAM_ADDR );
      tag_value = _lr( DC_TAG );
     
      /* If VALID and DIRTY then flush the line */
      if ( 
           (BIT_EXTRACT( tag_value, DC_TAG_V, DC_TAG_V_W ) == DC_TAG_V_VALID) &&
           (BIT_EXTRACT( tag_value, DC_TAG_D, DC_TAG_D_W ) == DC_TAG_D_DIRTY) 
         )
           
      { 

         /* Convert TAG value to an memory address by clearing line OFFSET, VALID, LOCKED, DIRTY bits */
         flsh_addr = tag_value & MASK( _dc_index_pos, (BITS_PER_ULONG - _dc_index_pos) );     
         _sr( flsh_addr, DC_FLDL );    /* Flush the cache line */
      
         /* Wait for Line Flush to complete */
          while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */
        
      }
   } 
   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings */
}

/*
---------------------------------------------------------------

  Purpose: Force coherency between the data cache and a memory region 


 _dcache_flush_mlines( void *mem_ptr, unsigned int mem_size )
 
     mem_ptr :  pointer to begining of memory region
     mem_size : size of memory region in bytes

  
-------------------------------------------------------------
*/
void _dcache_flush_mlines( void *mem_ptr, int mem_size )
{
   
   unsigned int old_ctrl,dc_control;
   unsigned int tag_value; 
   unsigned int flsh_addr;

   #ifdef debug
   store_flush[i+50] = (uint_32) mem_ptr;
   i++;
   #endif

   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings*/
   
    
  dc_control = old_ctrl;      /* Set Data Cache control to CACHE controlled TAG Access Mode */
  dc_control =  BIT_SET( dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_CACHE );
   _sr( dc_control, DC_CTRL ); 
   
   flsh_addr = (unsigned int)mem_ptr;
   do{ 
       /* Determine if line is in the D_cache*/
      _sr( flsh_addr, DC_RAM_ADDR);
      tag_value = _lr( DC_TAG );      
      if( BIT_EXTRACT( _lr(DC_CTRL), DC_CTRL_SB, DC_CTRL_SB_W) == DC_CTRL_SB_SUCCESS ) 
      {
         /* Do the line flush*/
         _sr( flsh_addr, DC_FLDL);
           while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */
      } 
      flsh_addr += _dc_line_size;
      
   }while( (mem_size -= _dc_line_size) > 0 );     

   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings   */
      
   
}

/*---------------------------------------------------------------

  Purpose: Force coherency between the data cache and a memory region 


 _dcache_flush_line( void *mem_ptr )
 
     mem_ptr :  address to line to flush

  
-------------------------------------------------------------*/
void _dcache_flush_line( void *mem_ptr)
{
   
   unsigned int old_ctrl, dc_control;
   unsigned int tag_value; 
   unsigned int flsh_addr;
   
   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings */
   
   /* Get Line size from D_cache BCR   */
         
   dc_control = old_ctrl;      /* Set Data Cache control to CACHE controlled TAG Access Mode */
   dc_control =  BIT_SET( dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_CACHE );
   _sr( dc_control, DC_CTRL ); 
   
   flsh_addr = (unsigned int)mem_ptr;
       /* Determine if line is in the D_cache */
      _sr( flsh_addr, DC_RAM_ADDR);
      tag_value = _lr( DC_TAG );      
      if( BIT_EXTRACT( _lr(DC_CTRL), DC_CTRL_SB, DC_CTRL_SB_W) == DC_CTRL_SB_SUCCESS ) 
      {
         /* Do the line flush */
         _sr( flsh_addr, DC_FLDL);
          while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */
      } 

   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings   */
      
   
}

/*Invalidate applies to possible zero of memory if line is not valid. Only invalidate valid lines.*/

/*---------------------------------------------------------------

  Purpose: Force coherency between the data cache and memory


 _dcache_invalidate(void)
 

  
-------------------------------------------------------------*/


void _dcache_invalidate( void )
{
   unsigned int old_ctrl,dc_control;
   unsigned int address, tag_addr, tag_value, inval_addr;
   
   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings */
   
      
  dc_control = old_ctrl;      /* Set Data Cache control to Direct TAG Access Mode */
  dc_control =  BIT_SET(dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_DIRECT );
   _sr(dc_control, DC_CTRL ); 
     

   /* Walk through all Line Index in all ways to check for VALID and DIRTY bits */
   for( address = (0x01 << (_dc_index_width + _dc_way_width)); address != 0; address-- )
   { 
      /* Get the TAG value from the cache */
      tag_addr = BIT_SET( 0x0, _dc_index_pos, (_dc_index_width + _dc_way_width), address );
      _sr( tag_addr, DC_RAM_ADDR );
      tag_value = _lr( DC_TAG );
     
      /* If VALID then invalidate the line  */
      if ( 
           (BIT_EXTRACT( tag_value, DC_TAG_V, DC_TAG_V_W ) == DC_TAG_V_VALID)
         )
           
      { 

         /* Convert TAG value to an memory address by clearing line OFFSET, VALID, LOCKED, DIRTY bits */
         inval_addr = tag_value & MASK( _dc_index_pos, (BITS_PER_ULONG - _dc_index_pos) );     
         _sr( inval_addr, DC_IVDL );    /* Invalidate the cache line */
      
         /* Wait for Line Flush to complete */
          while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */
        
      }
   } 
   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings */
}


/*---------------------------------------------------------------

  Purpose: Force coherency between the data cache and a memory region 


 _dcache_invalidate_mlines( void *mem_ptr, unsigned int mem_size )
 
     mem_ptr :  pointer to begining of memory region
     mem_size : size of memory region in bytes

  
-------------------------------------------------------------*/
void _dcache_invalidate_mlines( void *mem_ptr, int mem_size )
{
   
   unsigned int old_ctrl,dc_control;
   unsigned int tag_value; 
   unsigned int inval_addr;

   #ifdef debug
   store_flush[i++] = (uint_32) mem_ptr;
   #endif
      
   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings */
   
    
  dc_control = old_ctrl;      /* Set Data Cache control to CACHE controlled TAG Access Mode */
  dc_control =  BIT_SET( dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_CACHE );
   _sr( dc_control, DC_CTRL ); 
   
   inval_addr = (unsigned int)mem_ptr;
   do{ 
       /* Determine if line is in the D_cache */
      _sr( inval_addr, DC_RAM_ADDR);
      tag_value = _lr( DC_TAG );      
      if( BIT_EXTRACT( _lr(DC_CTRL), DC_CTRL_SB, DC_CTRL_SB_W) == DC_CTRL_SB_SUCCESS ) 
      {
         _sr( inval_addr, DC_IVDL );    /* Invalidate the cache line */
          while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */

      } 
      inval_addr += _dc_line_size;
      
   }while( (mem_size -= _dc_line_size) > 0 );     

   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings    */
      
   
}

/*---------------------------------------------------------------

  Purpose: Force coherency between the data cache and a memory region 


 _dcache_invalidate_line( void *mem_ptr )
 
     mem_ptr :  address to line to flush

  
-------------------------------------------------------------*/
void _dcache_invalidate_line( void *mem_ptr)
{
   
   unsigned int old_ctrl, dc_control;
   unsigned int tag_value; 
   unsigned int inval_addr;
   
   if (!_dc_index_pos )  _dcache_detect_config();

   old_ctrl = _lr( DC_CTRL );  /* Save Data Cache control settings */
   
   /* Get Line size from D_cache BCR    */
         
   dc_control = old_ctrl;      /* Set Data Cache control to CACHE controlled TAG Access Mode */
   dc_control =  BIT_SET( dc_control , DC_CTRL_AT , DC_CTRL_AT_W , DC_CTRL_AT_CACHE );
   _sr( dc_control, DC_CTRL ); 
   
   inval_addr = (unsigned int)mem_ptr;
       /* Determine if line is in the D_cache */
      _sr( inval_addr, DC_RAM_ADDR);
      tag_value = _lr( DC_TAG );      
      if( BIT_EXTRACT( _lr(DC_CTRL), DC_CTRL_SB, DC_CTRL_SB_W) == DC_CTRL_SB_SUCCESS ) 
      {
         _sr( inval_addr, DC_IVDL );    /* Invalidate the cache line */
          while(_lr(DC_CTRL) & DC_CTRL_FS) _nop();  /* poll flush status */
      } 

   _sr( old_ctrl, DC_CTRL );  /* Restore Data Cache Control Settings   */
      
   
}

#endif

extern int alloc_size;

void * USB_memalloc(uint_32 n)
{
#if 0
	void * unaligned_ptr = malloc(USB_CACHE_ALIGN(n) + PSP_CACHE_LINE_SIZE*2);
	uint_8_ptr aligned_ptr =  (uint_8_ptr) USB_CACHE_ALIGN( (uint_32) unaligned_ptr);
	uint_32_ptr temp = (uint_32_ptr) (aligned_ptr);
	(*temp) = (uint_32) unaligned_ptr;
	DEBUGMSG(USB_DBG_LEVEL,(" USBMemAlloc = %x\r\n", (aligned_ptr + PSP_CACHE_LINE_SIZE)));
	return (aligned_ptr + PSP_CACHE_LINE_SIZE);
	
#else
	void * aligned_ptr = mv_aligned_malloc(n, 32, HEAP_DDR );
	DEBUGMSG(USB_DBG_LEVEL,(" USBMemAlloc = %x\r\n", (aligned_ptr )));
	return aligned_ptr;
#endif

}

void  USB_memfree(void * aligned_ptr)
{
#if 0
	uint_32_ptr temp = (uint_32_ptr) ((uint_8_ptr)aligned_ptr - PSP_CACHE_LINE_SIZE);
	void * unaligned_ptr = (void *) (*temp);
	DEBUGMSG(USB_DBG_LEVEL,(" USBMemFree = %x\r\n", unaligned_ptr ));
	free(unaligned_ptr);
	return;
#else
	mv_aligned_free(aligned_ptr);
#endif
}
                                     
void * USB_Uncached_memalloc(uint_32 n)
{
#if 0
   void * unaligned_ptr = malloc(USB_CACHE_ALIGN(n) + PSP_CACHE_LINE_SIZE*2);
   uint_8_ptr aligned_ptr =  (uint_8_ptr) USB_CACHE_ALIGN( (uint_32) unaligned_ptr);
   uint_32_ptr temp = (uint_32_ptr) (aligned_ptr);
   (*temp) = (uint_32) unaligned_ptr;
   return (aligned_ptr + PSP_CACHE_LINE_SIZE);
#else
   void * aligned_ptr = mv_aligned_malloc(n, 32, HEAP_DDR_NONECACHE );
//   	DEBUGMSG(USB_DBG_LEVEL,(" USB_Uncached_memalloc = %x\r\n", (aligned_ptr )));
   	return aligned_ptr;
#endif
}




#endif /* __MQX__ */


/* EOF */
   
