/*******************************************************************************
@File
@Title          Server bridge for smm
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Implements the server side of the bridge for smm
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
********************************************************************************/

#include <linux/uaccess.h>

#include "img_defs.h"

#include "pmr.h"
#include "secure_export.h"


#include "common_smm_bridge.h"

#include "allocmem.h"
#include "pvr_debug.h"
#include "connection_server.h"
#include "pvr_bridge.h"
#if defined(SUPPORT_RGX)
#include "rgx_bridge.h"
#endif
#include "srvcore.h"
#include "handle.h"

#include <linux/slab.h>






/* ***************************************************************************
 * Server-side bridge entry points
 */
 
static IMG_INT
PVRSRVBridgePMRSecureExportPMR(IMG_UINT32 ui32DispatchTableEntry,
					  PVRSRV_BRIDGE_IN_PMRSECUREEXPORTPMR *psPMRSecureExportPMRIN,
					  PVRSRV_BRIDGE_OUT_PMRSECUREEXPORTPMR *psPMRSecureExportPMROUT,
					 CONNECTION_DATA *psConnection)
{
	IMG_HANDLE hPMR = psPMRSecureExportPMRIN->hPMR;
	PMR * psPMRInt = NULL;
	PMR * psPMROutInt = NULL;
	CONNECTION_DATA *psSecureConnection;







	/* Lock over handle lookup. */
	LockHandle();





					/* Look up the address from the handle */
					psPMRSecureExportPMROUT->eError =
						PVRSRVLookupHandleUnlocked(psConnection->psHandleBase,
											(void **) &psPMRInt,
											hPMR,
											PVRSRV_HANDLE_TYPE_PHYSMEM_PMR,
											IMG_TRUE);
					if(psPMRSecureExportPMROUT->eError != PVRSRV_OK)
					{
						UnlockHandle();
						goto PMRSecureExportPMR_exit;
					}
	/* Release now we have looked up handles. */
	UnlockHandle();

	psPMRSecureExportPMROUT->eError =
		PMRSecureExportPMR(psConnection, OSGetDevData(psConnection),
					psPMRInt,
					&psPMRSecureExportPMROUT->Export,
					&psPMROutInt, &psSecureConnection);
	/* Exit early if bridged call fails */
	if(psPMRSecureExportPMROUT->eError != PVRSRV_OK)
	{
		goto PMRSecureExportPMR_exit;
	}










PMRSecureExportPMR_exit:

	/* Lock over handle lookup cleanup. */
	LockHandle();







					/* Unreference the previously looked up handle */
					if(psPMRInt)
					{
						PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
										hPMR,
										PVRSRV_HANDLE_TYPE_PHYSMEM_PMR);
					}
	/* Release now we have cleaned up look up handles. */
	UnlockHandle();

	if (psPMRSecureExportPMROUT->eError != PVRSRV_OK)
	{
		if (psPMROutInt)
		{
			PMRSecureUnexportPMR(psPMROutInt);
		}
	}


	return 0;
}


static IMG_INT
PVRSRVBridgePMRSecureUnexportPMR(IMG_UINT32 ui32DispatchTableEntry,
					  PVRSRV_BRIDGE_IN_PMRSECUREUNEXPORTPMR *psPMRSecureUnexportPMRIN,
					  PVRSRV_BRIDGE_OUT_PMRSECUREUNEXPORTPMR *psPMRSecureUnexportPMROUT,
					 CONNECTION_DATA *psConnection)
{









	/* Lock over handle destruction. */
	LockHandle();





	psPMRSecureUnexportPMROUT->eError =
		PVRSRVReleaseHandleUnlocked(psConnection->psHandleBase,
					(IMG_HANDLE) psPMRSecureUnexportPMRIN->hPMR,
					PVRSRV_HANDLE_TYPE_PHYSMEM_PMR_SECURE_EXPORT);
	if ((psPMRSecureUnexportPMROUT->eError != PVRSRV_OK) &&
	    (psPMRSecureUnexportPMROUT->eError != PVRSRV_ERROR_RETRY))
	{
		PVR_DPF((PVR_DBG_ERROR,
		        "PVRSRVBridgePMRSecureUnexportPMR: %s",
		        PVRSRVGetErrorStringKM(psPMRSecureUnexportPMROUT->eError)));
		UnlockHandle();
		goto PMRSecureUnexportPMR_exit;
	}

	/* Release now we have destroyed handles. */
	UnlockHandle();



PMRSecureUnexportPMR_exit:




	return 0;
}


static IMG_INT
PVRSRVBridgePMRSecureImportPMR(IMG_UINT32 ui32DispatchTableEntry,
					  PVRSRV_BRIDGE_IN_PMRSECUREIMPORTPMR *psPMRSecureImportPMRIN,
					  PVRSRV_BRIDGE_OUT_PMRSECUREIMPORTPMR *psPMRSecureImportPMROUT,
					 CONNECTION_DATA *psConnection)
{
	PMR * psPMRInt = NULL;








	psPMRSecureImportPMROUT->eError =
		PMRSecureImportPMR(psConnection, OSGetDevData(psConnection),
					psPMRSecureImportPMRIN->Export,
					&psPMRInt,
					&psPMRSecureImportPMROUT->uiSize,
					&psPMRSecureImportPMROUT->sAlign);
	/* Exit early if bridged call fails */
	if(psPMRSecureImportPMROUT->eError != PVRSRV_OK)
	{
		goto PMRSecureImportPMR_exit;
	}

	/* Lock over handle creation. */
	LockHandle();





	psPMRSecureImportPMROUT->eError = PVRSRVAllocHandleUnlocked(psConnection->psHandleBase,

							&psPMRSecureImportPMROUT->hPMR,
							(void *) psPMRInt,
							PVRSRV_HANDLE_TYPE_PHYSMEM_PMR,
							PVRSRV_HANDLE_ALLOC_FLAG_MULTI
							,(PFN_HANDLE_RELEASE)&PMRUnrefPMR);
	if (psPMRSecureImportPMROUT->eError != PVRSRV_OK)
	{
		UnlockHandle();
		goto PMRSecureImportPMR_exit;
	}

	/* Release now we have created handles. */
	UnlockHandle();



PMRSecureImportPMR_exit:



	if (psPMRSecureImportPMROUT->eError != PVRSRV_OK)
	{
		if (psPMRInt)
		{
			PMRUnrefPMR(psPMRInt);
		}
	}


	return 0;
}




/* *************************************************************************** 
 * Server bridge dispatch related glue 
 */

static IMG_BOOL bUseLock = IMG_TRUE;

PVRSRV_ERROR InitSMMBridge(void);
PVRSRV_ERROR DeinitSMMBridge(void);

/*
 * Register all SMM functions with services
 */
PVRSRV_ERROR InitSMMBridge(void)
{

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREEXPORTPMR, PVRSRVBridgePMRSecureExportPMR,
					NULL, bUseLock);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREUNEXPORTPMR, PVRSRVBridgePMRSecureUnexportPMR,
					NULL, bUseLock);

	SetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREIMPORTPMR, PVRSRVBridgePMRSecureImportPMR,
					NULL, bUseLock);


	return PVRSRV_OK;
}

/*
 * Unregister all smm functions with services
 */
PVRSRV_ERROR DeinitSMMBridge(void)
{

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREEXPORTPMR);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREUNEXPORTPMR);

	UnsetDispatchTableEntry(PVRSRV_BRIDGE_SMM, PVRSRV_BRIDGE_SMM_PMRSECUREIMPORTPMR);



	return PVRSRV_OK;
}