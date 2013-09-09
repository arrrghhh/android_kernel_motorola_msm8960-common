/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined( __VossWrapperUtil_h__ )
#define __VossWrapperUtil_h__

/*===========================================================================
  @file VossWrapperUtil.h

  @brief Wrapper module to allow the use of VOSS utilities without
  changes to the legacy code.
  
  Copyright (c) 2008 QUALCOMM Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary
===========================================================================*/

/*=========================================================================== 
    
                       EDIT HISTORY FOR FILE 
   
   
  This section contains comments describing changes made to the module. 
  Notice that changes are listed in reverse chronological order. 
   
   
  $Header:$ $DateTime: $ $Author: $ 
   
   
  when        who    what, where, why 
  --------    ---    --------------------------------------------------------
  11/20/08    sho    Create this file from WinWrapperUtil.h; remove all
                     dependencies on WM platform and make this VOSS 
                     dependant only
     
===========================================================================*/ 

/*---------------------------------------------------------------------------
 * Include Files
 * ------------------------------------------------------------------------*/
#include "vos_types.h"
#include "vos_trace.h"
#include "vos_threads.h"
#include "vos_timer.h"

/*---------------------------------------------------------------------------
 * Type Declarations
 * ------------------------------------------------------------------------*/

typedef
v_VOID_t
(*VOSSWRAPPER_TIMER_FUNCTION) (
    v_PVOID_t                   SystemSpecific1,
    v_PVOID_t                  FunctionContext,
    v_PVOID_t                  SystemSpecific2,
    v_PVOID_t                   SystemSpecific3
    );


typedef enum sVossWrapperUtilTimerState
{//has to start with 0, which is the default after the object being created (zero memory).
    eANI_TIMER_STATE_UNINITIALIZED = 0,    
    eANI_TIMER_STATE_INACTIVE,
    eANI_TIMER_STATE_PENDING,
    eANI_TIMER_STATE_ACTIVE

} tVossWrapperUtilTimerState;

typedef struct sVossWrapperUtilTimer
{
    void *pAdapter;

    vos_timer_t timerObj;

    v_PVOID_t timerContext;

    VOSSWRAPPER_TIMER_FUNCTION timerFunc;

    tVossWrapperUtilTimerState timerState;

    v_BOOL_t    fTimerCancelled;

    v_BOOL_t fCurrentlyPeriodic;

    v_BOOL_t fCleanupPending;

} tVossWrapperUtilTimer, *tpVossWrapperUtilTimer;



// Timer utility routines and prototypes
VOS_INLINE_FN v_BOOL_t VossWrapperutilIsTimerActive( 
      tpVossWrapperUtilTimer pTimer )
{
  return( eANI_TIMER_STATE_ACTIVE == pTimer->timerState );
}

VOS_INLINE_FN v_BOOL_t VossWrapperutilIsTimerPending(
      tpVossWrapperUtilTimer pTimer )
{
  return( eANI_TIMER_STATE_PENDING == pTimer->timerState );
}

VOS_INLINE_FN v_BOOL_t VossWrapperutilIsTimerInactive( 
      tpVossWrapperUtilTimer pTimer )
{
  return( eANI_TIMER_STATE_INACTIVE == pTimer->timerState || 
        eANI_TIMER_STATE_UNINITIALIZED == pTimer->timerState);
}

VOS_INLINE_FN v_BOOL_t VossWrapperutilIsTimerCancelled( 
      tpVossWrapperUtilTimer pTimer )
{
  return( pTimer->fTimerCancelled );
}

VOS_INLINE_FN v_BOOL_t VossWrapperutilIsTimerCleanupPending( 
      tpVossWrapperUtilTimer pTimer )
{
  return( pTimer->fCleanupPending );
}

/*--------------------------------------------------------------------------
  
  \brief VossWrapperutilStartTimer() - Start the timer

  The function starts a vOSS timer. A timer must be initialized before it 
  can be started.
     
  A timer only runs once (a one-shot timer).  To re-start the 
  timer, the start has to be called after the timer runs or has been cancelled.

  
  \param pTimer - pointer to a structure which contains the vOOS timer object 
                  to be stopped
  \param mSecDelay - expiration time for the timer (in milliseconds)
  \param fPeriodic - boolean which tells if the timer is periodic. Since the 
                     rescheduleTimeInMsecs in TX_TIMER itself tells if the
                     timer is periodic or not this field is redundent, but leaving
                     it there as legacy purpose

  \return None
  
  \sa
  
  --------------------------------------------------------------------------*/
extern void VossWrapperutilStartTimer( tpVossWrapperUtilTimer pTimer, 
      v_UINT_t mSecDelay, v_BOOL_t fPeriodic );

/*--------------------------------------------------------------------------
  
  \brief VossWrapperutilCancelTimer() - Cancel the timer

  The timer can be cancelled only if it has been started. The function cancels a 
  timer that has been started but has not expired.
   
  After a timer is cancelled, it goes back to the state it was in after it
  was created in VossWrapperutilInitTimer() and can be started again via a call 
  to VossWrapperutilStartTimer().

  The difference between VossWrapperutilCancelTimer & VossWrapperutilHaltTimer is
  on VossWrapperutilHaltTimer in case the timer couldn't be stopped 
  successfully, it waits untill the timer state moves to 
  eANI_TIMER_STATE_INACTIVE
  
  \param pTimer - pointer to a structure which contains the vOOS timer object 
                  to be stopped

  \return TRUE - Timer is successfully stopped.
  
          FALSE - Timer couldn't be stopped cleanly.     
  \sa
  
  --------------------------------------------------------------------------*/
extern v_BOOL_t VossWrapperutilCancelTimer( tpVossWrapperUtilTimer pTimer );

/*--------------------------------------------------------------------------
  
  \brief VossWrapperutilHaltTimer() - Stop the timer

  The timer can be stopped only if it has been started. The function stops a 
  timer that has been started but has not expired.
   
  After a timer is stopped, it goes back to the state it was in after it
  was created in VossWrapperutilInitTimer() and can be started again via a call 
  to VossWrapperutilStartTimer().

  The difference between VossWrapperutilCancelTimer & VossWrapperutilHaltTimer is
  on VossWrapperutilHaltTimer in case the timer couldn't be stopped 
  successfully, it waits untill the timer state moves to 
  eANI_TIMER_STATE_INACTIVE
  
  \param pTimer - pointer to a structure which contains the vOOS timer object 
                  to be stopped

  \return TRUE - Timer is successfully stopped.
  
          FALSE - Timer couldn't be stopped cleanly.     
  \sa
  
  --------------------------------------------------------------------------*/
extern v_BOOL_t VossWrapperutilHaltTimer( tpVossWrapperUtilTimer pTimer );

/*--------------------------------------------------------------------------
  
  \brief VossWrapperutilInitTimer() - Initialize the timer

  The function initializes a vOSS timer. A timer must be initialized before it 
  may be used in any other timer functions.
     
  After initialization, the timer moves to the state from where it can be 
  started via a call to VossWrapperutilStartTimer().

  
  \param pTimer - pointer to a structure which contains the vOOS timer object 
                  to be initialized
  \param mpAdapterPtr - pointer to the pAdapter structure, needed for some of 
                        the internal routines
  \param timerFunc - pointer to the callback function(user supplied callback i.e.
                     client specific callback) registered through either  
                     VossWrapperutilResetTimer or VossWrapperutilInitTimer which 
                     will get called when the corresponding VoSS timer expires
  \param context - pointer to the timer conext, where the timer is of type
                   TX_TIMER

  \return None
  
  \sa
  
  --------------------------------------------------------------------------*/
extern void VossWrapperutilInitTimer( tpVossWrapperUtilTimer pTimer, 
      v_PVOID_t pAdapter, VOSSWRAPPER_TIMER_FUNCTION timerFunc, 
      v_PVOID_t context );

/*--------------------------------------------------------------------------
  
  \brief VossWrapperutilResetTimer() - Reset the timer

   Saves certain timer related information in the sVossWrapperUtilTimer 
   structure.

   Keeping this code as part of legacy code reference.
  
  \param pTimer - pointer to a structure which contains the vOOS timer object 
                  to be initialized
  \param mpAdapterPtr - pointer to the pAdapter structure, needed for some of 
                        the internal routines
  \param timerFunc - pointer to the callback function registered by either  
                     VossWrapperutilResetTimer or VossWrapperutilInitTimer which 
                     will get called when the corresponding VoSS timer expires
  \param context - pointer to the timer conext, where the timer is of type
                   TX_TIMER

  \return None
  
  \sa
  
  --------------------------------------------------------------------------*/
extern void VossWrapperutilResetTimer( tpVossWrapperUtilTimer pTimer, 
      v_PVOID_t pAdapter, VOSSWRAPPER_TIMER_FUNCTION timerFunc, 
      v_PVOID_t context );

#endif
