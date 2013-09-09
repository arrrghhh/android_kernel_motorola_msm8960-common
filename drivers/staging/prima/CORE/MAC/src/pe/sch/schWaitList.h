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

/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * This file schWaitList.h contains functions implementing the 
 * STA wait list
 *
 * Author:      Sandesh Goel
 * Date:        02/05/04
 * History:-
 * Date            Modified by    Modification Information
 * --------------------------------------------------------------------
 * 
 */

INLINE void
schDumpWaitList(tpAniSirGlobal pMac)
{
    tANI_U16 tot;
    tANI_U16 index;

    schLog(pMac, LOGW, FL("STA wait list : Num entries %d [%d,%d] Wait %d\n"), 
           pMac->sch.staWaitListCount, pMac->sch.staWaitListOut, 
           pMac->sch.staWaitListIn, pMac->sch.staWaitListTotalWait);
    tot = (pMac->sch.staWaitListCount > 20 ? 20 : pMac->sch.staWaitListCount);
    index = pMac->sch.staWaitListOut;
    while (tot)
    {
        schLog(pMac, LOGW, FL("\tEntry[%d] = %d,%d\n"), index, 
               pMac->sch.pStaWaitList[index].staId, pMac->sch.pStaWaitList[index].count);
        tot--;
        index = (index + 1) % pMac->lim.maxStation;
    }
}

// ---------------------------------------------------------------------------
/**
 * schStaWaitListInitialize
 *
 * FUNCTION: 
 * 
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

INLINE void 
schStaWaitListInitialize(tpAniSirGlobal pMac)
{
    pMac->sch.staWaitListCount = 0;
    pMac->sch.staWaitListIn = 0;
    pMac->sch.staWaitListOut = 0;
    pMac->sch.staWaitListTotalWait = 0;
}

// ---------------------------------------------------------------------------
/**
 * schSendSchCleanMsg
 *
 * FUNCTION: 
 * Send message to LIM indicating that it is okay to delete the sta now
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

INLINE void
schSendSchCleanMsg(tpAniSirGlobal pMac, tANI_U16 staId)
{
    tSirMsgQ msg;
    msg.type = SIR_LIM_SCH_CLEAN_MSG;
    msg.bodyval = staId;
    msg.bodyptr = NULL;
    limPostMsgApi(pMac, &msg);
    schLog(pMac, LOG1, FL("Sent SCH clean msg to LIM, sta %d\n"),
           staId);
}

// ---------------------------------------------------------------------------
/**
 * schStaWaitListAdd
 *
 * FUNCTION: 
 * 
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

INLINE void 
schStaWaitListAdd(tpAniSirGlobal pMac, tANI_U16 staId)
{
     tANI_U16 index;
     SIR_DECLARE_FLAGS;
     
     if (pMac->sch.schObject.gSchCFBInitiated ||
         pMac->sch.schObject.gSchCFPInitiated)
     {
        if (pMac->sch.staWaitListCount >= pMac->lim.maxStation)
            schLog(pMac, LOGP, FL("STA wait list full %d entries\n"), pMac->sch.staWaitListCount);
        
        TX_DISABLE_INTR;
        
        index = pMac->sch.staWaitListIn;   
        
        pMac->sch.pStaWaitList[index].staId = staId;
        pMac->sch.pStaWaitList[index].count = 2 - pMac->sch.staWaitListTotalWait;
        pMac->sch.staWaitListTotalWait = 2;

        pMac->sch.staWaitListIn = (index + 1) % pMac->lim.maxStation;
        pMac->sch.staWaitListCount++;
        
        TX_ENABLE_INTR;
        
        schLog(pMac, LOG1, FL("STA %d added to wait list[%d]\n"), 
               staId, pMac->sch.staWaitListCount);
     }
     else
         schSendSchCleanMsg(pMac, staId);
}

// ---------------------------------------------------------------------------
/**
 * schStaWaitListDecrement
 *
 * FUNCTION: 
 * 
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

INLINE void
schStaWaitListDecrement(tpAniSirGlobal pMac)
{
    SIR_DECLARE_FLAGS;

    if (pMac->sch.staWaitListCount == 0) return;
    
    TX_DISABLE_INTR;

    if (pMac->sch.pStaWaitList[pMac->sch.staWaitListOut].count == 0 ||
        pMac->sch.staWaitListTotalWait == 0)
    {
        schDumpWaitList(pMac);
        schLog(pMac, LOGP, FL("schStaWaitListDecrement: Inconsistent state\n"));
    }

    pMac->sch.pStaWaitList[pMac->sch.staWaitListOut].count--;
    pMac->sch.staWaitListTotalWait--;

    while (pMac->sch.staWaitListCount &&
           pMac->sch.pStaWaitList[pMac->sch.staWaitListOut].count == 0)
    {
        tANI_U16 staId = pMac->sch.pStaWaitList[pMac->sch.staWaitListOut].staId;

#if 0   //BTAMP: commented out to compile. Need to clean this up       
        tpDphHashNode pSta = dphGetHashEntry(pMac, staId);
        if (pSta == NULL)
        {
            schDumpWaitList(pMac);
            schLog(pMac, LOGP, FL("schStaWaitListDecrement: STA %d not found\n"), staId);
        }
#endif            
        schSendSchCleanMsg(pMac, staId);
        
        pMac->sch.staWaitListOut = (pMac->sch.staWaitListOut + 1) % pMac->lim.maxStation;
        pMac->sch.staWaitListCount--;
    }

    TX_ENABLE_INTR;
}

// ---------------------------------------------------------------------------
/**
 * schStaWaitListClear
 *
 * FUNCTION: 
 * 
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param None
 * @return None
 */

INLINE void
schStaWaitListClear(tpAniSirGlobal pMac)
{
    schStaWaitListDecrement(pMac);
    schStaWaitListDecrement(pMac);
}

// ---------------------------------------------------------------------------

