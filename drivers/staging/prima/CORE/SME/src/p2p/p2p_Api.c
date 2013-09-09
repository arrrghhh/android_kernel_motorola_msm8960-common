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
 * Copyright (c) 2008 QUALCOMM Incorporated. All Rights Reserved.
 * Qualcomm Confidential and Proprietary 
 */

#if defined WLAN_FEATURE_P2P

#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
#include "halInternal.h"
#endif
#include "sme_Api.h"
#include "smsDebug.h"
#include "csrInsideApi.h"
#include "smeInside.h"
#include "p2p_Api.h"
#include "limApi.h"


/*------------------------------------------------------------------
 *
 * handle SME remain on channel request.
 *
 *------------------------------------------------------------------*/

eHalStatus p2pProcessRemainOnChannelCmd(tpAniSirGlobal pMac, tSmeCmd *p2pRemainonChn)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tSirRemainOnChnReq* pMsg;
    tANI_U16 len;
    tCsrRoamSession *pSession = CSR_GET_SESSION( pMac, p2pRemainonChn->sessionId );

    if(!pSession)
    {
       smsLog(pMac, LOGE, FL("  session %d not found "), p2pRemainonChn->sessionId);
       return eHAL_STATUS_FAILURE;
    }

    if( !pSession->sessionActive ) VOS_ASSERT(0);

    len = sizeof(tSirRemainOnChnReq) + pMac->p2pContext.probeRspIeLength;
    
    status = palAllocateMemory(pMac->hHdd, (void**)&pMsg, len );
    if(HAL_STATUS_SUCCESS(status))
    {
        palZeroMemory(pMac->hHdd, pMsg, sizeof(tSirRemainOnChnReq));
        pMsg->messageType = eWNI_SME_REMAIN_ON_CHANNEL_REQ;
        pMsg->length = len;
        palCopyMemory( pMac, pMsg->selfMacAddr, pSession->selfMacAddr, sizeof(tSirMacAddr) ); 
        pMsg->chnNum = p2pRemainonChn->u.remainChlCmd.chn;
        pMsg->duration = p2pRemainonChn->u.remainChlCmd.duration;
        pMsg->sessionId = p2pRemainonChn->sessionId;
        if( pMac->p2pContext.probeRspIeLength )
            palCopyMemory(pMac->hHdd, (void *)pMsg->probeRspIe, (void *)pMac->p2pContext.probeRspIe, pMac->p2pContext.probeRspIeLength);
    
        status = palSendMBMessage(pMac->hHdd, pMsg);
    }
    
    return status;
}

/*------------------------------------------------------------------
 *
 * handle LIM remain on channel rsp: Success/failure.
 *
 *------------------------------------------------------------------*/

eHalStatus sme_remainOnChnRsp( tpAniSirGlobal pMac, tANI_U8 *pMsg)
{
    eHalStatus                         status = eHAL_STATUS_SUCCESS;
    tListElem                          *pEntry = NULL;
    tSmeCmd                            *pCommand = NULL;

    pEntry = csrLLPeekHead(&pMac->sme.smeCmdActiveList, LL_ACCESS_LOCK);
    if( pEntry )
    {
        pCommand = GET_BASE_ADDR(pEntry, tSmeCmd, Link);
        if( eSmeCommandRemainOnChannel == pCommand->command )
        {
            remainOnChanCallback callback = pCommand->u.remainChlCmd.callback;
            /* process the msg */
            if( callback )
                callback(pMac, pCommand->u.remainChlCmd.callbackCtx, 0);
             
            if( csrLLRemoveEntry( &pMac->sme.smeCmdActiveList, pEntry, LL_ACCESS_LOCK ) )
            {
                //Now put this command back on the avilable command list
                smeReleaseCommand(pMac, pCommand);
            }
            smeProcessPendingQueue( pMac );
        }
    }
    return status;
}

/*------------------------------------------------------------------
 *
 * Handle the Mgmt frm ind from LIM and forward to HDD.
 *
 *------------------------------------------------------------------*/

eHalStatus sme_mgmtFrmInd( tHalHandle hHal, tpSirSmeMgmtFrameInd pSmeMgmtFrm)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    eHalStatus  status = eHAL_STATUS_SUCCESS;
    tCsrRoamInfo pRoamInfo = {0};
    tANI_U32 pSessionId = pSmeMgmtFrm->sessionId;

    pRoamInfo.nFrameLength = pSmeMgmtFrm->mesgLen - sizeof(tSirSmeMgmtFrameInd);
    pRoamInfo.pbFrames = pSmeMgmtFrm->frameBuf;
    pRoamInfo.frameType = pSmeMgmtFrm->frameType;
    pRoamInfo.rxChan   = pSmeMgmtFrm->rxChan;

    /* forward the mgmt frame to HDD */
    csrRoamCallCallback(pMac, pSessionId, &pRoamInfo, 0, eCSR_ROAM_INDICATE_MGMT_FRAME, 0);

    return status;
}

/*------------------------------------------------------------------
 *
 * Handle the remain on channel ready indication from PE
 *
 *------------------------------------------------------------------*/

eHalStatus sme_remainOnChnReady( tHalHandle hHal, tANI_U8* pMsg)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    eHalStatus  status = eHAL_STATUS_SUCCESS;
    tCsrRoamInfo RoamInfo; 
    tListElem                          *pEntry = NULL;
    tSmeCmd                            *pCommand = NULL;
  
    pEntry = csrLLPeekHead(&pMac->sme.smeCmdActiveList, LL_ACCESS_LOCK);
    if( pEntry )
    {
        pCommand = GET_BASE_ADDR(pEntry, tSmeCmd, Link);
        if( eSmeCommandRemainOnChannel == pCommand->command )
        {
            /* forward the indication to HDD */
            RoamInfo.pRemainCtx = pCommand->u.remainChlCmd.callbackCtx;
            csrRoamCallCallback(pMac, ((tSirSmeRsp*)pMsg)->sessionId, &RoamInfo, 
                                0, eCSR_ROAM_REMAIN_CHAN_READY, 0);
        }
    }
  
    return status;
}

eHalStatus sme_sendActionCnf( tHalHandle hHal, tANI_U8* pMsg)
{
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );
    eHalStatus  status = eHAL_STATUS_SUCCESS;
    tCsrRoamInfo RoamInfo = {0};
    tSirSmeRsp* pSmeRsp = (tSirSmeRsp*)pMsg;
  
    /* forward the indication to HDD */
    //RoamInfo can be passed as NULL....todo
    csrRoamCallCallback(pMac, pSmeRsp->sessionId, &RoamInfo, 0, 
                        eCSR_ROAM_SEND_ACTION_CNF, 
                       (pSmeRsp->statusCode == eSIR_SME_SUCCESS) ? 0:
                        eCSR_ROAM_RESULT_SEND_ACTION_FAIL);
    
    return status;
}
eHalStatus sme_p2pOpen( tHalHandle hHal )
{
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
  
    //If static structure is too big, Need to change this function to allocate memory dynamically
    vos_mem_zero( &pMac->p2pContext, sizeof( tp2pContext ) );
    return eHAL_STATUS_SUCCESS;
}

eHalStatus p2pStop( tHalHandle hHal )
{
    return eHAL_STATUS_SUCCESS;
}

eHalStatus sme_p2pClose( tHalHandle hHal )
{
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
  
    if( pMac->p2pContext.probeRspIe )
        vos_mem_free( pMac->p2pContext.probeRspIe );
  
    pMac->p2pContext.probeRspIeLength = 0;
  
    return eHAL_STATUS_SUCCESS;
}

eHalStatus p2pRemainOnChannel(tHalHandle hHal, tANI_U8 sessionId,
         tANI_U8 channel, tANI_U32 duration,
        remainOnChanCallback callback, 
        void *pContext)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
    tSmeCmd *pRemainChlCmd = NULL;
  
    pRemainChlCmd = smeGetCommandBuffer(pMac);
    if(pRemainChlCmd == NULL)
        return eHAL_STATUS_FAILURE;
  
    do
    {
        /* call set in context */
        pRemainChlCmd->command = eSmeCommandRemainOnChannel;
        pRemainChlCmd->sessionId = sessionId;
        pRemainChlCmd->u.remainChlCmd.chn = channel;
        pRemainChlCmd->u.remainChlCmd.duration = duration;
        pRemainChlCmd->u.remainChlCmd.callback = callback;
        pRemainChlCmd->u.remainChlCmd.callbackCtx = pContext;
    
        csrQueueSmeCommand(pMac, pRemainChlCmd, eANI_BOOLEAN_FALSE);
    } while(0);
  
    smsLog(pMac, LOGW, "exiting function %s\n", __FUNCTION__);
  
    return(status);
}

eHalStatus p2pSendAction(tHalHandle hHal, tANI_U8 sessionId,
         const tANI_U8 *pBuf, tANI_U32 len, tANI_U16 wait, tANI_BOOLEAN noack)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
    tSirMbMsgP2p *pMsg;
    tANI_U16 msgLen;

    msgLen = (tANI_U16)((sizeof( tSirMbMsg )) + len);
    status = palAllocateMemory(pMac->hHdd, (void **)&pMsg, msgLen);
    if(HAL_STATUS_SUCCESS(status))
    {
        palZeroMemory(pMac->hHdd, (void *)pMsg, msgLen);
        pMsg->type = pal_cpu_to_be16((tANI_U16)eWNI_SME_SEND_ACTION_FRAME_IND);
        pMsg->msgLen = pal_cpu_to_be16(msgLen);
        pMsg->sessionId = sessionId;
        pMsg->noack = noack;
        pMsg->wait = (tANI_U16)wait;
        palCopyMemory( pMac->hHdd, pMsg->data, pBuf, len );
        status = palSendMBMessage(pMac->hHdd, pMsg);
    }

    return( status );
}

eHalStatus p2pCancelRemainOnChannel(tHalHandle hHal, tANI_U8 sessionId)
{
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT(hHal);
    tSirMbMsg *pMsg;
    tANI_U16 msgLen;

    msgLen = (tANI_U16)(sizeof( tSirMbMsg ));
    status = palAllocateMemory(pMac->hHdd, (void **)&pMsg, msgLen);
    if(HAL_STATUS_SUCCESS(status))
    {
        palZeroMemory(pMac->hHdd, (void *)pMsg, msgLen);
        pMsg->type = pal_cpu_to_be16((tANI_U16)eWNI_SME_ABORT_REMAIN_ON_CHAN_IND);
        pMsg->msgLen = pal_cpu_to_be16(msgLen);
        status = palSendMBMessage(pMac->hHdd, pMsg);
    }                             

    return( status );
}

eHalStatus p2pSetPs(tHalHandle hHal, tP2pPsConfig *pNoA)
{
    tpP2pPsConfig pNoAParam;
    tSirMsgQ msg;
    eHalStatus status = eHAL_STATUS_SUCCESS;
    tpAniSirGlobal pMac = PMAC_STRUCT( hHal );

    status = palAllocateMemory(pMac->hHdd, (void**)&pNoAParam, sizeof(tP2pPsConfig));
    if(HAL_STATUS_SUCCESS(status))
    {
        palZeroMemory(pMac->hHdd, pNoAParam, sizeof(tP2pPsConfig));
        palCopyMemory(pMac->hHdd, pNoAParam, pNoA, sizeof(tP2pPsConfig)); 
        msg.type = eWNI_SME_UPDATE_NOA;
        msg.bodyval = 0;
        msg.bodyptr = pNoAParam;
        limPostMsgApi(pMac, &msg);
    }   
    return status;
}

eHalStatus p2pProcessNoAReq(tpAniSirGlobal pMac, tSmeCmd *pNoACmd)
{
    tpP2pPsConfig pNoA;
    tSirMsgQ msg;
    eHalStatus status = eHAL_STATUS_SUCCESS;

    status = palAllocateMemory(pMac->hHdd, (void**)&pNoA, sizeof(tP2pPsConfig));
    if(HAL_STATUS_SUCCESS(status))
    {
        palZeroMemory(pMac->hHdd, pNoA, sizeof(tP2pPsConfig));
        pNoA->opp_ps = pNoACmd->u.NoACmd.NoA.opp_ps;
        pNoA->ctWindow = pNoACmd->u.NoACmd.NoA.ctWindow;
        pNoA->duration = pNoACmd->u.NoACmd.NoA.duration;
        pNoA->interval = pNoACmd->u.NoACmd.NoA.interval;
        pNoA->count = pNoACmd->u.NoACmd.NoA.count;
        pNoA->single_noa_duration = pNoACmd->u.NoACmd.NoA.single_noa_duration;
        pNoA->psSelection = pNoACmd->u.NoACmd.NoA.psSelection;
        pNoA->sessionid = pNoACmd->u.NoACmd.NoA.sessionid;
        msg.type = eWNI_SME_UPDATE_NOA;
        msg.bodyval = 0;
        msg.bodyptr = pNoA;
        limPostMsgApi(pMac, &msg);
    }   
    return status;
}

#endif
