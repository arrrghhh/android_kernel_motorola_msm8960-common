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
 * Airgo Networks, Inc proprietary. All rights reserved
 * sysStartup.h: System startup header file.
 *
 * Author:      V. K. Kandarpa
 * Date:        07/26/2002
 *
 * History:-
 * Date         Modified by            Modification Information
 * --------------------------------------------------------------------------
 *
 */

# ifndef __SYSRTAISTARTUP_H
# define __SYSRTAISTARTUP_H

#include <sirCommon.h>
#include <aniParam.h>
#include "halTypes.h"
extern void sysRecvPacket(tHalHandle hHal, void* pPacket);
extern void sysRtaiStartup(tAniMacParam*);
extern void sysRtaiCleanup(tHalHandle hHal);
extern void sysMacModInit(tAniMacParam * pParam, struct rtLibApp * rt);
extern void sysMacModExit(tHalHandle hHal);
extern eHalStatus sysMailboxRead(tHalHandle hHal, void * message);
extern eHalStatus sysMailboxWrite(tHalHandle hHal, void * message);
extern int mac_mod_init(tAniMacParam * pParam);
extern void mac_mod_exit(tAniMacParam * pParam);

# endif /* __SYSSTARTUP_H */
