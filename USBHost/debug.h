#ifndef __host_debug_h__
#define __host_debug_h__
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
***   This file contains definitions for debugging the software stack
***                                                               
**************************************************************************
**END*********************************************************/
#include "usbh_types.h"

/*--------------------------------------------------------------**
** Debug macros, assume _DBUG_ is defined during development    **
**    (perhaps in the make file), and undefined for production. **
**--------------------------------------------------------------*/
#ifndef _DBUG_
   #define DEBUG_FLUSH
   #define DEBUG_PRINT(X)
   #define DEBUG_PRINT2(X,Y)
#else
   #define DEBUG_FLUSH        fflush(stdout);
   #define DEBUG_PRINT(X)     printf(X); 
   #define DEBUG_PRINT2(X,Y)  printf(X,Y);
#endif


/*--------------------------------------------------------------**
** ASSERT macros, assume _ASSERT_ is defined during development **
**    (perhaps in the make file), and undefined for production. **
** This macro will check for pointer values and other validations
** wherever appropriate.
**--------------------------------------------------------------*/
#ifndef _ASSERT_
   #define ASSERT(X,Y)
#else
   #define ASSERT(X,Y) if(Y) { printf(X); exit(1);}
#endif


/************************************************************
The following array is used to make a run time trace route
inside the USB stack.
*************************************************************/

#ifdef _DEBUG_INFO_TRACE_LEVEL_

#define TRACE_ARRAY_SIZE 1000
#define MAX_STRING_SIZE  50

#ifndef __TRACE_VARIABLES_DEFINED__
extern uint_16 DEBUG_TRACE_ARRAY_COUNTER;
extern char   DEBUG_TRACE_ARRAY[TRACE_ARRAY_SIZE][MAX_STRING_SIZE];
extern boolean DEBUG_TRACE_ENABLED;
#else
uint_16 DEBUG_TRACE_ARRAY_COUNTER;
char   DEBUG_TRACE_ARRAY[TRACE_ARRAY_SIZE][MAX_STRING_SIZE];
boolean DEBUG_TRACE_ENABLED;
#endif


#define DEBUG_LOG_TRACE(x) \
{ \
  if(DEBUG_TRACE_ENABLED) \
 { \
	 USB_memcopy(x,DEBUG_TRACE_ARRAY[DEBUG_TRACE_ARRAY_COUNTER],MAX_STRING_SIZE);\
	 DEBUG_TRACE_ARRAY_COUNTER++;\
	 if(DEBUG_TRACE_ARRAY_COUNTER >= TRACE_ARRAY_SIZE) \
	 {DEBUG_TRACE_ARRAY_COUNTER = 0;}\
 }\
}
#define START_DEBUG_TRACE \
{ \
	DEBUG_TRACE_ARRAY_COUNTER =0;\
	DEBUG_TRACE_ENABLED = TRUE;\
} 

#define STOP_DEBUG_TRACE \
{ \
	DEBUG_TRACE_ENABLED = FALSE;\
} 

/*if trace switch is not enabled define debug log trace to empty*/
#else
	#define DEBUG_LOG_TRACE(x)
	#define START_DEBUG_TRACE
	#define STOP_DEBUG_TRACE
#endif


/************************************************************
The following are global data structures that can be used
to copy data from stack on run time. This structure can
be analyzed at run time to see the state of various other
data structures in the memory.
*************************************************************/


#ifdef _DEBUG_INFO_DATA_LEVEL_

typedef struct debug_data {
} DEBUG_DATA_STRUCT,_PTR_ DEBUG_DATA_STRUCT_PTR;

#endif

/**************************************************************
	The following lines assign numbers to each of the routines
	in the stack. These numbers can be used to generate a trace
	on sequenece of routines entered.
**************************************************************/



#endif
/* EOF */
