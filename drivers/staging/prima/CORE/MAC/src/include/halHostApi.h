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

/**
 *
   Airgo Networks, Inc proprietary.
   All Rights Reserved, Copyright 2005
   This program is the confidential and proprietary product of Airgo Networks Inc.
   Any Unauthorized use, reproduction or transfer of this program is strictly prohibited.


   halhostApi.h: Interfaces that the host system provides
   Author:  Mark Nelson
   Date:    4/9/05

   History -
   Date        Modified by              Modification Information
  --------------------------------------------------------------------------

 */

#ifndef HALHOSTAPI_H
#define HALHOSTAPI_H

#include "sirTypes.h"

#if (WNI_POLARIS_FW_OS == SIR_WINDOWS)
#include "hddHW.h"
#elif !defined(ANI_AP_SDK)
#include "rtaiWrapper.h"
#endif




#ifdef ANI_PCI_HANG_WORKAROUND
# define HAL_MIN_BURST_SIZE        0       // in words
#else
# define HAL_MIN_BURST_SIZE        4       // in words
#endif









// Windows specific macros for the HIF Burst RW implementation
#if (WNI_POLARIS_FW_OS == SIR_WINDOWS)


#define halSetInternalPolHostIntrMask(_pMac, polIntrMask) \
                     hwSetDefaultHostIntrMask(_pMac->pAdapter, polIntrMask)

#endif //(WNI_POLARIS_FW_OS == SIR_WINDOWS)






/// HIF block read command interface
extern tSirRetStatus halHifBurstRead(void *, tANI_U32*, tANI_U32*, tANI_U16);

// ------------------------------------------------------------------
/**
 * pChip Source address from Polaris memories
 * pDst  Destination pointer in PCI address
 * n     Number of words
 */

static inline tSirRetStatus
halMemRead(tpAniSirGlobal pMac, tANI_U32* pDst, tANI_U32* pChip, tANI_U32 n)
{
    tSirRetStatus rc = eSIR_SUCCESS;
    return rc;
}


// ------------------------------------------------------------------
/**
 * pChip Destination address from Polaris memories
 * pSrc Source pointer in PCI address
 * n Number of words
 */

static inline tSirRetStatus
halMemWrite(tpAniSirGlobal pMac, tANI_U32* pChip, tANI_U32* pSrc, tANI_U32 n)
{
    tSirRetStatus rc = eSIR_SUCCESS;

    return rc;
}

// ------------------------------------------------------------------
/**
 * pChip Destination address from Polaris memories
 * pat Pattern to be populated in the pChip memory
 * n Number of words
 */

static inline void
halMemWritePattern(tpAniSirGlobal pMac, tANI_U32* pChip, tANI_U32 pat, tANI_S32 n)
{
}


#if (WNI_POLARIS_FW_OS == SIR_WINDOWS)

// Stub out halEnableInterrupts for the Windows implementation
#define halEnableInterrupts()

// Ported version of halSetReg static inline function (see halApi.h for original function).
// This macro assumes that the function calling it has a tANI_ADAPTER *pAdapter
// pointer passed into it, with a pointer to the HDD Adapter instance structure.
static inline void
halSetReg(tpAniSirGlobal pMac, tANI_U32 reg, tANI_U32 val)
{
}

// Ported version of halGetR static inline function (see halApi.h for original function).
// This macro assumes that the function calling it has a tANI_ADAPTER *pAdapter
// pointer passed into it, with a pointer to the HDD Adapter instance structure.
static inline tANI_U32
halGetReg(tpAniSirGlobal pMac, tANI_U32 reg)
{
    return 0;
}

#else

extern unsigned long gBAR0_addr;

/**
 * halSetReg
 *
 * FUNCTION:
 *     This interface shall be used to write to Polaris register.
 *     This is an static inline function
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *     This interface is used to set the registers as well as some
 *     memories. The reason for this is that reigsters are also in memory
 *     space in Polaris.
 *     In case of accessing memories, it will be accessed 4 bytes at a time.
 *
 * @param tpAniSirGlobal MAC parameter structure pointer
 * @param reg Register absolute address
 * @param val Register value to be set
 * @return None
 */

static inline void
halSetRegNoBurst(tpAniSirGlobal pMac, tANI_U32 reg, tANI_U32 val)
{

} // halSetReg

static inline void
halSetReg(tpAniSirGlobal pMac, tANI_U32 reg, tANI_U32 val)
{
    halSetRegNoBurst(pMac, reg, val);
}

// -------------------------------------------------------------
/**
 * halGetRegNoBurst
 *
 * FUNCTION:
 *     This interface is used to read from Polaris register.
 *     This is an static inline function.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *     This interface is used to get the registers as well as some memories.
 *     The reason for this is that reigsters are also in memory space
 *     in Polaris.
 *     In case of accessing memories, it will be accessed 4 bytes at a time.
 *
 * @param tpAniSirGlobal MAC parameter structure pointer
 * @param reg reg is register absolute address
 * @return register contents
 */

static inline tANI_U32
halGetRegNoBurst(tpAniSirGlobal pMac, tANI_U32 reg)
{
    tANI_U32 b = 0;
    return b;
} // halGetReg

// -------------------------------------------------------------
/**
 * halGetReg
 *
 * FUNCTION:
 *     This interface is used to read from Polaris register.
 *     This is an static inline function.
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *     This interface is used to get the registers as well as some memories.
 *     The reason for this is that reigsters are also in memory space
 *     in Polaris.
 *     In case of accessing memories, it will be accessed 4 bytes at a time.
 *
 * @param tpAniSirGlobal MAC parameter structure pointer
 * @param reg reg is register absolute address
 * @return register contents
 */
static inline tANI_U32
halGetReg(tpAniSirGlobal pMac, tANI_U32 reg)
{
    tANI_U32 b = 0;
    return b;
}

#endif // (WNI_POLARIS_FW_OS == SIR_WINDOWS)


#endif /* HALHOSTAPI_H */
