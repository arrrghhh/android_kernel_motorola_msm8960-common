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
 * This file schBeaconProcess.cc contains beacon processing related
 * functions
 *
 * Author:      Sandesh Goel
 * Date:        02/25/02
 * History:-
 * Date            Modified by    Modification Information
 * --------------------------------------------------------------------
 *
 */

#include "palTypes.h"
#include "wniCfgAp.h"

#include "cfgApi.h"
#include "pmmApi.h"
#include "limApi.h"
#include "utilsApi.h"
#include "schDebug.h"
#include "schApi.h"

#ifdef FEATURE_WLAN_NON_INTEGRATED_SOC
#include "halCommonApi.h"
#endif

#include "limUtils.h"
#include "limSendMessages.h"
#include "limStaHashApi.h"

#if defined WLAN_FEATURE_VOWIFI
#include "rrmApi.h"
#endif

#ifdef FEATURE_WLAN_DIAG_SUPPORT
#include "vos_diag_core_log.h"
#endif //FEATURE_WLAN_DIAG_SUPPORT 

/**
 * Number of bytes of variation in beacon length from the last beacon
 * to trigger reprogramming of rx delay register
 */
#define SCH_BEACON_LEN_DELTA       3

// calculate 2^cw - 1
#define CW_GET(cw) (((cw) == 0) ? 1 : ((1 << (cw)) - 1))

static void
ap_beacon_process(
    tpAniSirGlobal    pMac,
    tANI_U8*      pRxPacketInfo,
    tpSchBeaconStruct pBcnStruct,
    tpUpdateBeaconParams pBeaconParams,
    tpPESession         psessionEntry)
{
    tpSirMacMgmtHdr    pMh = WDA_GET_RX_MAC_HEADER(pRxPacketInfo);
    tANI_U32           phyMode;
    tSirRFBand          rfBand = SIR_BAND_UNKNOWN;
    //Get RF band from psessionEntry
    rfBand = psessionEntry->limRFBand;
    limGetPhyMode(pMac, &phyMode);    
    if(SIR_BAND_5_GHZ == rfBand)
    {
        if (psessionEntry->htCapabality)
        {
            if (pBcnStruct->channelNumber == psessionEntry->currentOperChannel)
            {
              //11a (non HT) AP  overlaps or
              //HT AP with HT op mode as mixed overlaps.              
              //HT AP with HT op mode as overlap legacy overlaps.                            
              if ((!pBcnStruct->HTInfo.present) ||
                  (eSIR_HT_OP_MODE_MIXED == pBcnStruct->HTInfo.opMode) ||
                  (eSIR_HT_OP_MODE_OVERLAP_LEGACY == pBcnStruct->HTInfo.opMode))
              {
                   limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOverlap11aParams));

                  if (pMac->lim.gLimOverlap11aParams.numSta &&
                      !pMac->lim.gLimOverlap11aParams.protectionEnabled)
                  {
                      limEnable11aProtection(pMac, true, true, pBeaconParams,psessionEntry);
                  }
              }
              //HT AP with HT20 op mode overlaps.
              else if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == pBcnStruct->HTInfo.opMode)
              {
                  limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOverlapHt20Params));

                  if (pMac->lim.gLimOverlapHt20Params.numSta &&
                      !pMac->lim.gLimOverlapHt20Params.protectionEnabled)
                  {
                      limEnableHT20Protection(pMac, true, true, pBeaconParams,psessionEntry);
                  }
              }
            }
        }    
    }
    else if(SIR_BAND_2_4_GHZ == rfBand)
    {
        //We are 11G AP.
        if ((phyMode == WNI_CFG_PHY_MODE_11G) &&
              (false == psessionEntry->htCapabality))
        {
            if (pBcnStruct->channelNumber == psessionEntry->currentOperChannel)        
            {
                if (((!(pBcnStruct->erpPresent)) && 
                      !(pBcnStruct->HTInfo.present))|| 
                    //if erp not present then  11B AP overlapping
                    (pBcnStruct->erpPresent &&
                    (pBcnStruct->erpIEInfo.useProtection ||
                    pBcnStruct->erpIEInfo.nonErpPresent)))
                {
                    limEnableOverlap11gProtection(pMac, pBeaconParams, pMh,psessionEntry);
                }

            }
        }        
        // handling the case when HT AP has overlapping legacy BSS.
        else if(psessionEntry->htCapabality)
        {
            if (pBcnStruct->channelNumber == psessionEntry->currentOperChannel)
            {
              if (pBcnStruct->erpPresent &&
                    (pBcnStruct->erpIEInfo.useProtection ||
                    pBcnStruct->erpIEInfo.nonErpPresent))
              {
                  limEnableOverlap11gProtection(pMac, pBeaconParams, pMh,psessionEntry);
              }

              //11g device overlaps
              if (pBcnStruct->erpPresent &&
                  !(pBcnStruct->erpIEInfo.useProtection || 
                    pBcnStruct->erpIEInfo.nonErpPresent) && !(pBcnStruct->HTInfo.present))
              {
#ifdef WLAN_SOFTAP_FEATURE
                    limUpdateOverlapStaParam(pMac, pMh->bssId, &(psessionEntry->gLimOverlap11gParams));

                  if (psessionEntry->gLimOverlap11gParams.numSta && 
                      !psessionEntry->gLimOverlap11gParams.protectionEnabled)
#else
                   limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOverlap11gParams));

                  if (pMac->lim.gLimOverlap11gParams.numSta &&
                      !pMac->lim.gLimOverlap11gParams.protectionEnabled)
#endif
                  {
                      limEnableHtProtectionFrom11g(pMac, true, true, pBeaconParams,psessionEntry);
                  }
              }

              //ht device overlaps.
              //here we will check for HT related devices only which might need protection.
              //check for 11b and 11g is already done in the previous blocks.
              //so we will not check for HT operating mode as MIXED.
              if (pBcnStruct->HTInfo.present)
              {
                  //if we are not already in mixed mode or legacy mode as HT operating mode
                  //and received beacon has HT operating mode as legacy
                  //then we need to enable protection from 11g station. 
                  //we don't need protection from 11b because if that's needed then our operating
                  //mode would have already been set to legacy in the previous blocks.
                  if(eSIR_HT_OP_MODE_OVERLAP_LEGACY == pBcnStruct->HTInfo.opMode)
                  {
                      if((eSIR_HT_OP_MODE_MIXED != pMac->lim.gHTOperMode) &&
                          (eSIR_HT_OP_MODE_OVERLAP_LEGACY != pMac->lim.gHTOperMode))
                      {
#ifdef WLAN_SOFTAP_FEATURE
                          limUpdateOverlapStaParam(pMac, pMh->bssId, &(psessionEntry->gLimOverlap11gParams));
                          if (psessionEntry->gLimOverlap11gParams.numSta &&
                              !psessionEntry->gLimOverlap11gParams.protectionEnabled)
#else
                          limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOverlap11gParams));

                          if (pMac->lim.gLimOverlap11gParams.numSta &&
                              !pMac->lim.gLimOverlap11gParams.protectionEnabled)
#endif
                          {
                              limEnableHtProtectionFrom11g(pMac, true, true, pBeaconParams,psessionEntry);
                          }
                      }
                  }           
                  else if(eSIR_HT_OP_MODE_NO_LEGACY_20MHZ_HT == pBcnStruct->HTInfo.opMode)
                  {
#ifdef WLAN_SOFTAP_FEATURE
                      limUpdateOverlapStaParam(pMac, pMh->bssId, &(psessionEntry->gLimOverlapHt20Params));
                      if (psessionEntry->gLimOverlapHt20Params.numSta &&
                          !psessionEntry->gLimOverlapHt20Params.protectionEnabled)
#else
                      limUpdateOverlapStaParam(pMac, pMh->bssId, &(pMac->lim.gLimOverlapHt20Params));

                      if (pMac->lim.gLimOverlapHt20Params.numSta &&
                          !pMac->lim.gLimOverlapHt20Params.protectionEnabled)
#endif
                      {
                          limEnableHT20Protection(pMac, true, true, pBeaconParams,psessionEntry);
                      }
                  }
              }
              
            }
        }     
    }
    pMac->sch.gSchBcnIgnored++;
}
// --------------------------------------------------------------------




/**
 * __schBeaconProcessNoSession
 *
 * FUNCTION:
 * Process the received beacon frame when 
 *  -- Station is not scanning 
 *  -- No corresponding session is found
 *
 * LOGIC:
 *        Following scenarios exist when Session Does not exist:
 *             * IBSS Beacons, when IBSS session already exists with same SSID, 
 *                but from STA which has not yet joined and has a different BSSID.
 *                - invoke limHandleIBSScoalescing with the session context of existing IBSS session.
 *
 *             * IBSS Beacons when IBSS session does not exist, only Infra or BT-AMP session exists,
 *                then save the beacon in the scan results and throw it away.
 *                
 *             * Infra Beacons
 *                - beacons received when no session active 
 *                    should not come here, it should be handled as part of scanning, 
 *                    else they should not be getting received, should update scan results and drop it if that happens.
 *                - beacons received when IBSS session active:
 *                    update scan results and drop it.
 *                - beacons received when Infra session(STA) is active:
 *                    update scan results and drop it
 *                - beacons received when BT-STA session is active:
 *                    update scan results and drop it.
 *                - beacons received when Infra/BT-STA  or Infra/IBSS is active.
 *                    update scan results and drop it.
 * 

 */
static void __schBeaconProcessNoSession(tpAniSirGlobal pMac, tpSchBeaconStruct pBeacon,tANI_U8* pRxPacketInfo)
{
    tpPESession psessionEntry = NULL;
 
    if(  (psessionEntry = limIsIBSSSessionActive(pMac)) != NULL)
    {
        limHandleIBSScoalescing(pMac, pBeacon, pRxPacketInfo, psessionEntry);
    }

    //If station(STA/BT-STA/BT-AP/IBSS) mode, Always save the beacon in the scan results, if atleast one session is active
    //schBeaconProcessNoSession will be called only when there is atleast one session active, so not checking 
    //it again here.
    limCheckAndAddBssDescription(pMac, pBeacon, pRxPacketInfo, eANI_BOOLEAN_FALSE);
    return;  
}



/*
 * __schBeaconProcessForSession
 *
 * FUNCTION:
 * Process the received beacon frame when 
 *  -- Station is not scanning 
 *  -- Corresponding session is found
 *
 * LOGIC:
 *        Following scenarios exist when Session exists
 *             * IBSS STA receving beacons from IBSS Peers, who are part of IBSS.
 *                 - call limHandleIBSScoalescing with that session context.
 *             * Infra STA receving beacons from AP to which it is connected
 *                 - call schBeaconProcessFromAP with that session's context.
 *             * BTAMP STA receving beacons from BTAMP AP
 *                 - call schBeaconProcessFromAP with that session's context.
 *             * BTAMP AP receiving beacons from BTAMP STA 
 *               (here need to make sure BTAP creates session entry for BT STA)
 *                - just update the beacon count for heart beat purposes for now, 
 *                  for now, don't process the beacon.
 *             * Infra/IBSS both active and receives IBSS beacon:
 *                  - call limHandleIBSScoalescing with that session context.
 *             * Infra/IBSS both active and receives Infra beacon:
 *                  - call schBeaconProcessFromAP with that session's context.
 *                     any updates to EDCA parameters will be effective for IBSS as well, 
 *                     even though no WMM for IBSS ?? Need to figure out how to handle this scenario.
 *             * Infra/BTSTA both active and receive Infra beacon.
 *                  - change in EDCA parameters on Infra affect the BTSTA link.
 *                     Update the same parameters on BT link
 *              * Infra/BTSTA both active and receive BT-AP beacon.
 *                 -update beacon cnt for heartbeat
 *             * Infra/BTAP both active and receive Infra beacon.
 *                 - BT-AP starts advertising BE parameters from Infra AP, if they get changed.
 *
 *             * Infra/BTAP both active and receive BTSTA beacon.
 *                - update beacon cnt for heartbeat
 */

static void __schBeaconProcessForSession( tpAniSirGlobal      pMac,
                                                                     tpSchBeaconStruct   pBeacon,
                                                                     tANI_U8* pRxPacketInfo,    
                                                                     tpPESession psessionEntry)
{
    tANI_U32                     bi;
    tANI_U8 bssIdx = 0;
    //tpSirMacMgmtHdr         pMh = SIR_MAC_BD_TO_MPDUHEADER(pRxPacketInfo);
    //tANI_U8 bssid[sizeof(tSirMacAddr)];
    tUpdateBeaconParams beaconParams;
    tANI_U8 sendProbeReq = FALSE;
    tpDphHashNode pStaDs = NULL;


    beaconParams.paramChangeBitmap = 0;

    if(eLIM_STA_IN_IBSS_ROLE == psessionEntry->limSystemRole )
    {
        limHandleIBSScoalescing(pMac, pBeacon,  pRxPacketInfo, psessionEntry);
    }
    else if(  (eLIM_STA_ROLE == psessionEntry->limSystemRole) || 
                  (eLIM_BT_AMP_STA_ROLE == psessionEntry->limSystemRole))
    {
        /*
        *  This handles two cases:
        *  -- Infra STA receving beacons from AP  
        *  -- BTAMP_STA receving beacons from BTAMP_AP
        */
        
    
        //Always save the beacon into LIM's cached scan results
        limCheckAndAddBssDescription(pMac, pBeacon, pRxPacketInfo, eANI_BOOLEAN_FALSE);
        
        /**
               * This is the Beacon received from the AP  we're currently associated with. Check
               * if there are any changes in AP's capabilities 
               */
        if((tANI_U8) pBeacon->channelNumber != psessionEntry->currentOperChannel)
        {
            PELOGE(limLog(pMac, LOGE, FL("Channel Change from %d --> %d  - "
                                         "Ignoring beacon!\n"), 
                          psessionEntry->currentOperChannel, pBeacon->channelNumber);)
           goto fail;
        }
        limDetectChangeInApCapabilities(pMac, pBeacon, psessionEntry);
        if(limGetStaHashBssidx(pMac, DPH_STA_HASH_INDEX_PEER, &bssIdx, psessionEntry) != eSIR_SUCCESS)
            goto fail;
        beaconParams.bssIdx = bssIdx;
        palCopyMemory( pMac->hHdd, ( tANI_U8* )&psessionEntry->lastBeaconTimeStamp, ( tANI_U8* )pBeacon->timeStamp, sizeof(tANI_U64) );
        psessionEntry->lastBeaconDtimCount = pBeacon->tim.dtimCount;
        psessionEntry->lastBeaconDtimPeriod= pBeacon->tim.dtimPeriod;
        psessionEntry->currentBssBeaconCnt++;



        MTRACE(macTrace(pMac, TRACE_CODE_RX_MGMT_TSF, 0, pBeacon->timeStamp[0]);)
        MTRACE(macTrace(pMac, TRACE_CODE_RX_MGMT_TSF, 0, pBeacon->timeStamp[1]);)

        /* Read beacon interval session Entry */
        bi = psessionEntry->beaconParams.beaconInterval;
        if (bi != pBeacon->beaconInterval)
        {
           PELOG1(schLog(pMac, LOG1, FL("Beacon interval changed from %d to %d\n"),
                   pBeacon->beaconInterval, bi);)

            bi = pBeacon->beaconInterval;
            psessionEntry->beaconParams.beaconInterval = (tANI_U16) bi;
            beaconParams.paramChangeBitmap |= PARAM_BCN_INTERVAL_CHANGED;
            beaconParams.beaconInterval = (tANI_U16)bi;
        }

        if (pBeacon->cfPresent)
        {
            cfgSetInt(pMac, WNI_CFG_CFP_PERIOD, pBeacon->cfParamSet.cfpPeriod);
            limSendCFParams(pMac, bssIdx, pBeacon->cfParamSet.cfpCount, pBeacon->cfParamSet.cfpPeriod);
        }

        if (pBeacon->timPresent)
        {
            cfgSetInt(pMac, WNI_CFG_DTIM_PERIOD, pBeacon->tim.dtimPeriod);
            //No need to send DTIM Period and Count to HAL/SMAC
            //SMAC already parses TIM bit.
        }

        
        if(pMac->lim.gLimProtectionControl != WNI_CFG_FORCE_POLICY_PROTECTION_DISABLE)

        limDecideStaProtection(pMac, pBeacon, &beaconParams, psessionEntry);
        if (pBeacon->erpPresent)
        {
#ifdef WLAN_SOFTAP_FEATURE
            if (pBeacon->erpIEInfo.barkerPreambleMode)
                limEnableShortPreamble(pMac, false, &beaconParams, psessionEntry);
            else
                limEnableShortPreamble(pMac, true, &beaconParams, psessionEntry);
#else
            if (pBeacon->erpIEInfo.barkerPreambleMode)
                limEnableShortPreamble(pMac, false, &beaconParams);
            else
                limEnableShortPreamble(pMac, true, &beaconParams);
#endif
          }
        limUpdateShortSlot(pMac, pBeacon, &beaconParams,psessionEntry);

        pStaDs = dphGetHashEntry(pMac, DPH_STA_HASH_INDEX_PEER, &psessionEntry->dph.dphHashTable);
        if ((pBeacon->wmeEdcaPresent && (psessionEntry->limWmeEnabled)) ||
             (pBeacon->edcaPresent    && (psessionEntry->limQosEnabled)))
        {
            if(pBeacon->edcaParams.qosInfo.count != psessionEntry->gLimEdcaParamSetCount)
            {
                if (schBeaconEdcaProcess(pMac, &pBeacon->edcaParams, psessionEntry) != eSIR_SUCCESS)
                    PELOGE(schLog(pMac, LOGE, FL("EDCA parameter processing error\n"));)
                else if(pStaDs != NULL)
                {
                    // If needed, downgrade the EDCA parameters
                    limSetActiveEdcaParams(pMac, psessionEntry->gLimEdcaParams, psessionEntry); 

                    if (pStaDs->aniPeer == eANI_BOOLEAN_TRUE)
                        limSendEdcaParams(pMac, psessionEntry->gLimEdcaParamsActive, pStaDs->bssId, eANI_BOOLEAN_TRUE);
                    else
                        limSendEdcaParams(pMac, psessionEntry->gLimEdcaParamsActive, pStaDs->bssId, eANI_BOOLEAN_FALSE);
                }
                else
                    PELOGE(limLog(pMac, LOGE, FL("Self Entry missing in Hash Table\n"));)
            }
        }
        else if( (pBeacon->qosCapabilityPresent && psessionEntry->limQosEnabled) &&
            (pBeacon->qosCapability.qosInfo.count != psessionEntry->gLimEdcaParamSetCount))
            sendProbeReq = TRUE;
    }

    if ( pMac->lim.htCapability && pBeacon->HTInfo.present )
    {
        limUpdateStaRunTimeHTSwitchChnlParams( pMac, &pBeacon->HTInfo, bssIdx,psessionEntry);
    }

#if defined(ANI_PRODUCT_TYPE_CLIENT) || defined(ANI_AP_CLIENT_SDK)
    if ( (psessionEntry->limSystemRole == eLIM_STA_ROLE) ||(psessionEntry->limSystemRole == eLIM_BT_AMP_STA_ROLE) ||
          (psessionEntry->limSystemRole == eLIM_STA_IN_IBSS_ROLE) )
    {
        if(pBeacon->quietIEPresent)
        {
            limUpdateQuietIEFromBeacon(pMac, &(pBeacon->quietIE), psessionEntry);
        }
        else if ((pMac->lim.gLimSpecMgmt.quietState == eLIM_QUIET_BEGIN) ||
             (pMac->lim.gLimSpecMgmt.quietState == eLIM_QUIET_RUNNING))
        {
            PELOG1(limLog(pMac, LOG1, FL("Received a beacon without Quiet IE\n"));)
            limCancelDot11hQuiet(pMac, psessionEntry);
        }

        /* Channel Switch information element updated */
        if(pBeacon->channelSwitchPresent || 
            pBeacon->propIEinfo.propChannelSwitchPresent)
        {
            limUpdateChannelSwitch(pMac, pBeacon, psessionEntry);
        }
        else if (pMac->lim.gLimSpecMgmt.dot11hChanSwState == eLIM_11H_CHANSW_RUNNING)
        {
            limCancelDot11hChannelSwitch(pMac, psessionEntry);
        }   
    }
#endif

#if defined WLAN_FEATURE_VOWIFI
        if( pMac->rrm.rrmPEContext.rrmEnable )
        {
           tPowerdBm  localConstraint = 0, regMax = 0, maxTxPower = 0;
           if (pBeacon->powerConstraintPresent && pMac->rrm.rrmPEContext.rrmEnable)
           {
              localConstraint = pBeacon->localPowerConstraint.localPowerConstraints;
           }
           else
           {
              localConstraint = 0;
           }
           regMax = cfgGetRegulatoryMaxTransmitPower( pMac, psessionEntry->currentOperChannel ); 
           maxTxPower = VOS_MIN( regMax , (regMax - localConstraint) );
           //If maxTxPower is increased or decreased
           if( maxTxPower != psessionEntry->maxTxPower )
           {
#if defined WLAN_VOWIFI_DEBUG
              limLog( pMac, LOGE, "Regulatory max = %d, local power constraint = %d, max tx = %d", regMax, localConstraint, maxTxPower );
              limLog( pMac, LOGE, "Local power constraint change..updating mew maxTx power to HAL");
#endif
              if( rrmSendSetMaxTxPowerReq ( pMac, maxTxPower, psessionEntry ) == eSIR_SUCCESS )
                 psessionEntry->maxTxPower = maxTxPower;

           }
        }
#endif

    // Indicate to LIM that Beacon is received

    if (pBeacon->HTInfo.present)
        limReceivedHBHandler(pMac, (tANI_U8)pBeacon->HTInfo.primaryChannel, psessionEntry);
    else
        limReceivedHBHandler(pMac, (tANI_U8)pBeacon->channelNumber, psessionEntry);

    // I don't know if any additional IE is required here. Currently, not include addIE.
    if(sendProbeReq)
        limSendProbeReqMgmtFrame(pMac, &psessionEntry->ssId,
            psessionEntry->bssId, psessionEntry->currentOperChannel,psessionEntry->selfMacAddr,
            psessionEntry->dot11mode, 0, NULL);

   PELOG2(schLog(pMac, LOG2, "Received Beacon's SeqNum=%d\n",
           (pMh->seqControl.seqNumHi << 4) | (pMh->seqControl.seqNumLo));)

    if(beaconParams.paramChangeBitmap)
    {
        PELOGW(schLog(pMac, LOGW, FL("Beacon for session[%d] got changed. \n"), psessionEntry->peSessionId);)
        PELOGW(schLog(pMac, LOGW, FL("sending beacon param change bitmap: 0x%x \n"), beaconParams.paramChangeBitmap);)
        limSendBeaconParams(pMac, &beaconParams, psessionEntry);
    }

fail:
    return;

}



/**
 * schBeaconProcess
 *
 * FUNCTION:
 * Process the received beacon frame
 *
 * LOGIC:
  *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param pRxPacketInfo pointer to buffer descriptor
 * @return None
 */
 
void schBeaconProcess(tpAniSirGlobal pMac, tANI_U8* pRxPacketInfo, tpPESession psessionEntry)
{
    static tSchBeaconStruct beaconStruct;
    tUpdateBeaconParams beaconParams;
    tpPESession pAPSession = NULL;
    beaconParams.paramChangeBitmap = 0;

    pMac->sch.gSchBcnRcvCnt++;

    // Convert the beacon frame into a structure
    if (sirConvertBeaconFrame2Struct(pMac, (tANI_U8 *) pRxPacketInfo, &beaconStruct)!= eSIR_SUCCESS)
    {
        PELOGE(schLog(pMac, LOGE, FL("beacon parsing failed\n"));)
        pMac->sch.gSchBcnParseErrorCnt++;
        return;
    }

    if (beaconStruct.ssidPresent)
    {
        beaconStruct.ssId.ssId[beaconStruct.ssId.length] = 0;
    }

    /*
    * First process the beacon in the context of any existing AP or BTAP session.
    * This takes cares of following two scenarios:
    *  - psessionEntry = NULL:  
    *      e.g. beacon received from a neighboring BSS, you want to apply the protection settings to BTAP/InfraAP beacons
    *  - psessionEntry is non NULL: 
    *      e.g. beacon received is from the INFRA AP to which you are connected on another concurrent link.
    *      In this case also, we want to apply the protection settings(as advertised by Infra AP) to BTAP beacons
    * 
    * 
    */
    
    if((pAPSession = limIsApSessionActive(pMac)) != NULL)
    {
        beaconParams.bssIdx = pAPSession->bssIdx;
#ifdef WLAN_SOFTAP_FEATURE
        if (pAPSession->gLimProtectionControl != WNI_CFG_FORCE_POLICY_PROTECTION_DISABLE)
#else
        if (pMac->lim.gLimProtectionControl != WNI_CFG_FORCE_POLICY_PROTECTION_DISABLE)
#endif
            ap_beacon_process(pMac,  pRxPacketInfo, &beaconStruct, &beaconParams, pAPSession);

        if (beaconParams.paramChangeBitmap)
        {
            //Update the beacons and apply the new settings to HAL
            schSetFixedBeaconFields(pMac, pAPSession);
            PELOG1(schLog(pMac, LOG1, FL("Beacon for PE session[%d] got changed.  \n"), pAPSession->peSessionId);)
            PELOG1(schLog(pMac, LOG1, FL("sending beacon param change bitmap: 0x%x \n"), beaconParams.paramChangeBitmap);)
            limSendBeaconParams(pMac, &beaconParams, pAPSession);
        }
    }

    /*
    * Now process the beacon in the context of the BSS which is transmitting the beacons, if one is found
    */
    if(psessionEntry == NULL)
    {
        __schBeaconProcessNoSession(pMac,   &beaconStruct, pRxPacketInfo );   
    }
    else
    {
        __schBeaconProcessForSession(pMac,   &beaconStruct, pRxPacketInfo, psessionEntry );   
    }

}





// --------------------------------------------------------------------
/**
 * schBeaconEdcaProcess
 *
 * FUNCTION:
 * Process the EDCA parameter set in the received beacon frame
 *
 * LOGIC:
 *
 * ASSUMPTIONS:
 *
 * NOTE:
 *
 * @param edca reference to edca parameters in beacon struct
 * @return success
 */

tSirRetStatus schBeaconEdcaProcess(tpAniSirGlobal pMac, tSirMacEdcaParamSetIE *edca, tpPESession psessionEntry)
{
    tANI_U8 i;
#ifdef FEATURE_WLAN_DIAG_SUPPORT
    vos_log_qos_edca_pkt_type *log_ptr = NULL;
#endif //FEATURE_WLAN_DIAG_SUPPORT 

    PELOG1(schLog(pMac, LOG1, FL("Updating parameter set count: Old %d ---> new %d\n"),
           psessionEntry->gLimEdcaParamSetCount, edca->qosInfo.count);)

    psessionEntry->gLimEdcaParamSetCount = edca->qosInfo.count;
    psessionEntry->gLimEdcaParams[EDCA_AC_BE] = edca->acbe;
    psessionEntry->gLimEdcaParams[EDCA_AC_BK] = edca->acbk;
    psessionEntry->gLimEdcaParams[EDCA_AC_VI] = edca->acvi;
    psessionEntry->gLimEdcaParams[EDCA_AC_VO] = edca->acvo;
//log: LOG_WLAN_QOS_EDCA_C
#ifdef FEATURE_WLAN_DIAG_SUPPORT
    WLAN_VOS_DIAG_LOG_ALLOC(log_ptr, vos_log_qos_edca_pkt_type, LOG_WLAN_QOS_EDCA_C);
    if(log_ptr)
    {
       log_ptr->aci_be = psessionEntry->gLimEdcaParams[EDCA_AC_BE].aci.aci;
       log_ptr->cw_be  = psessionEntry->gLimEdcaParams[EDCA_AC_BE].cw.max << 4 |
          psessionEntry->gLimEdcaParams[EDCA_AC_BE].cw.min;
       log_ptr->txoplimit_be = psessionEntry->gLimEdcaParams[EDCA_AC_BE].txoplimit;
       log_ptr->aci_bk = psessionEntry->gLimEdcaParams[EDCA_AC_BK].aci.aci;
       log_ptr->cw_bk  = psessionEntry->gLimEdcaParams[EDCA_AC_BK].cw.max << 4 |
          psessionEntry->gLimEdcaParams[EDCA_AC_BK].cw.min;
       log_ptr->txoplimit_bk = psessionEntry->gLimEdcaParams[EDCA_AC_BK].txoplimit;
       log_ptr->aci_vi = psessionEntry->gLimEdcaParams[EDCA_AC_VI].aci.aci;
       log_ptr->cw_vi  = psessionEntry->gLimEdcaParams[EDCA_AC_VI].cw.max << 4 |
          psessionEntry->gLimEdcaParams[EDCA_AC_VI].cw.min;
       log_ptr->txoplimit_vi = psessionEntry->gLimEdcaParams[EDCA_AC_VI].txoplimit;
       log_ptr->aci_vo = psessionEntry->gLimEdcaParams[EDCA_AC_VO].aci.aci;
       log_ptr->cw_vo  = psessionEntry->gLimEdcaParams[EDCA_AC_VO].cw.max << 4 |
          psessionEntry->gLimEdcaParams[EDCA_AC_VO].cw.min;
       log_ptr->txoplimit_vo = psessionEntry->gLimEdcaParams[EDCA_AC_VO].txoplimit;
    }
    WLAN_VOS_DIAG_LOG_REPORT(log_ptr);
#endif //FEATURE_WLAN_DIAG_SUPPORT
    PELOG1(schLog(pMac, LOGE, FL("Updating Local EDCA Params(gLimEdcaParams) to: "));)
    for(i=0; i<MAX_NUM_AC; i++)
    {
        PELOG1(schLog(pMac, LOG1, FL("AC[%d]:  AIFSN: %d, ACM %d, CWmin %d, CWmax %d, TxOp %d\n"),
            i,
            psessionEntry->gLimEdcaParams[i].aci.aifsn, 
            psessionEntry->gLimEdcaParams[i].aci.acm,
            psessionEntry->gLimEdcaParams[i].cw.min,
            psessionEntry->gLimEdcaParams[i].cw.max,
            psessionEntry->gLimEdcaParams[i].txoplimit);)
    }

    return eSIR_SUCCESS;
}
