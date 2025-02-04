/*************************************************************************/ /*!
@File
@Title          Debug Functionality
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description	Provides kernel side Debug Functionality.
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
*/ /**************************************************************************/

#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/hardirq.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/slab.h>

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
 #include <linux/stdarg.h>
#else
 #include <stdarg.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) */

#include "allocmem.h"
#include "pvrversion.h"
#include "img_types.h"
#include "img_defs.h"
#include "servicesext.h"
#include "pvr_debug.h"
#include "srvkm.h"
#include "pvr_debugfs.h"
#include "linkage.h"
#include "pvr_uaccess.h"
#include "pvrsrv.h"
#include "lists.h"
#include "osfunc.h"

#include "rgx_options.h"

#if defined(SUPPORT_RGX)
#include "rgxdevice.h"
#include "rgxdebug.h"
#include "rgxinit.h"
#include "rgxfwutils.h"
#include "sofunc_rgx.h"
/* Handle used by DebugFS to get GPU utilisation stats */
static IMG_HANDLE ghGpuUtilUserDebugFS;
#endif

#if defined(PVRSRV_NEED_PVR_DPF)

/******** BUFFERED LOG MESSAGES ********/

/* Because we don't want to have to handle CCB wrapping, each buffered
 * message is rounded up to PVRSRV_DEBUG_CCB_MESG_MAX bytes. This means
 * there is the same fixed number of messages that can be stored,
 * regardless of message length.
 */

#if defined(PVRSRV_DEBUG_CCB_MAX)

#define PVRSRV_DEBUG_CCB_MESG_MAX	PVR_MAX_DEBUG_MESSAGE_LEN

#include <linux/syscalls.h>
#include <linux/time.h>

typedef struct
{
	const IMG_CHAR *pszFile;
	IMG_INT iLine;
	IMG_UINT32 ui32TID;
	IMG_UINT32 ui32PID;
	IMG_CHAR pcMesg[PVRSRV_DEBUG_CCB_MESG_MAX];
	struct timeval sTimeVal;
}
PVRSRV_DEBUG_CCB;

static PVRSRV_DEBUG_CCB gsDebugCCB[PVRSRV_DEBUG_CCB_MAX];

static IMG_UINT giOffset;

static DEFINE_MUTEX(gsDebugCCBMutex);

static void
AddToBufferCCB(const IMG_CHAR *pszFileName, IMG_UINT32 ui32Line,
			   const IMG_CHAR *szBuffer)
{
	mutex_lock(&gsDebugCCBMutex);

	gsDebugCCB[giOffset].pszFile = pszFileName;
	gsDebugCCB[giOffset].iLine   = ui32Line;
	gsDebugCCB[giOffset].ui32TID = current->pid;
	gsDebugCCB[giOffset].ui32PID = current->tgid;

	do_gettimeofday(&gsDebugCCB[giOffset].sTimeVal);

	strncpy(gsDebugCCB[giOffset].pcMesg, szBuffer, PVRSRV_DEBUG_CCB_MESG_MAX - 1);
	gsDebugCCB[giOffset].pcMesg[PVRSRV_DEBUG_CCB_MESG_MAX - 1] = 0;

	giOffset = (giOffset + 1) % PVRSRV_DEBUG_CCB_MAX;

	mutex_unlock(&gsDebugCCBMutex);
}

void PVRSRVDebugPrintfDumpCCB(void)
{
	int i;

	mutex_lock(&gsDebugCCBMutex);

	for (i = 0; i < PVRSRV_DEBUG_CCB_MAX; i++)
	{
		PVRSRV_DEBUG_CCB *psDebugCCBEntry =
			&gsDebugCCB[(giOffset + i) % PVRSRV_DEBUG_CCB_MAX];

		/* Early on, we won't have PVRSRV_DEBUG_CCB_MAX messages */
		if (!psDebugCCBEntry->pszFile)
		{
			continue;
		}

		printk(KERN_ERR "%s:%d: (%ld.%ld, tid=%u, pid=%u) %s\n",
			   psDebugCCBEntry->pszFile,
			   psDebugCCBEntry->iLine,
			   (long)psDebugCCBEntry->sTimeVal.tv_sec,
			   (long)psDebugCCBEntry->sTimeVal.tv_usec,
			   psDebugCCBEntry->ui32TID,
			   psDebugCCBEntry->ui32PID,
			   psDebugCCBEntry->pcMesg);

		/* Clear this entry so it doesn't get printed the next time again. */
		psDebugCCBEntry->pszFile = NULL;
	}

	mutex_unlock(&gsDebugCCBMutex);
}

#else /* defined(PVRSRV_DEBUG_CCB_MAX) */
static INLINE void
AddToBufferCCB(const IMG_CHAR *pszFileName, IMG_UINT32 ui32Line,
			   const IMG_CHAR *szBuffer)
{
	(void)pszFileName;
	(void)szBuffer;
	(void)ui32Line;
}

void PVRSRVDebugPrintfDumpCCB(void)
{
	/* Not available */
}

#endif /* defined(PVRSRV_DEBUG_CCB_MAX) */

#endif /* defined(PVRSRV_NEED_PVR_DPF) */

#if defined(PVRSRV_NEED_PVR_DPF)

#define PVR_MAX_FILEPATH_LEN 256

#if !defined(PVR_TESTING_UTILS)
static
#endif
IMG_UINT32 gPVRDebugLevel =
	(
	 DBGPRIV_FATAL | DBGPRIV_ERROR | DBGPRIV_WARNING

#if defined(PVRSRV_DEBUG_CCB_MAX)
	 | DBGPRIV_BUFFERED
#endif /* defined(PVRSRV_DEBUG_CCB_MAX) */

#if defined(PVR_DPF_ADHOC_DEBUG_ON)
	 | DBGPRIV_DEBUG
#endif /* defined(PVR_DPF_ADHOC_DEBUG_ON) */
	);

module_param(gPVRDebugLevel, uint, 0644);
MODULE_PARM_DESC(gPVRDebugLevel,
				 "Sets the level of debug output (default 0x7)");

#endif /* defined(PVRSRV_NEED_PVR_DPF) || defined(PVRSRV_NEED_PVR_TRACE) */

#define	PVR_MAX_MSG_LEN PVR_MAX_DEBUG_MESSAGE_LEN

/* Message buffer for non-IRQ messages */
static IMG_CHAR gszBufferNonIRQ[PVR_MAX_MSG_LEN + 1];

/* Message buffer for IRQ messages */
static IMG_CHAR gszBufferIRQ[PVR_MAX_MSG_LEN + 1];

/* The lock is used to control access to gszBufferNonIRQ */
static DEFINE_MUTEX(gsDebugMutexNonIRQ);

/* The lock is used to control access to gszBufferIRQ */
static DEFINE_SPINLOCK(gsDebugLockIRQ);

#define	USE_SPIN_LOCK (in_interrupt() || !preemptible())

static inline void GetBufferLock(unsigned long *pulLockFlags)
{
	if (USE_SPIN_LOCK)
	{
		spin_lock_irqsave(&gsDebugLockIRQ, *pulLockFlags);
	}
	else
	{
		__acquire(&gsDebugLockIRQ);
		mutex_lock(&gsDebugMutexNonIRQ);
	}
}

static inline void ReleaseBufferLock(unsigned long ulLockFlags)
{
	if (USE_SPIN_LOCK)
	{
		spin_unlock_irqrestore(&gsDebugLockIRQ, ulLockFlags);
	}
	else
	{
		__release(&gsDebugLockIRQ);
		mutex_unlock(&gsDebugMutexNonIRQ);
	}
}

static inline void SelectBuffer(IMG_CHAR **ppszBuf, IMG_UINT32 *pui32BufSiz)
{
	if (USE_SPIN_LOCK)
	{
		*ppszBuf = gszBufferIRQ;
		*pui32BufSiz = sizeof(gszBufferIRQ);
	}
	else
	{
		*ppszBuf = gszBufferNonIRQ;
		*pui32BufSiz = sizeof(gszBufferNonIRQ);
	}
}

/*
 * Append a string to a buffer using formatted conversion.
 * The function takes a variable number of arguments, pointed
 * to by the var args list.
 */
__printf(3, 0)
static IMG_BOOL VBAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz, const IMG_CHAR *pszFormat, va_list VArgs)
{
	IMG_UINT32 ui32Used;
	IMG_UINT32 ui32Space;
	IMG_INT32 i32Len;

	ui32Used = strlen(pszBuf);
	BUG_ON(ui32Used >= ui32BufSiz);
	ui32Space = ui32BufSiz - ui32Used;

	i32Len = vsnprintf(&pszBuf[ui32Used], ui32Space, pszFormat, VArgs);
	pszBuf[ui32BufSiz - 1] = 0;

	/* Return true if string was truncated */
	return i32Len < 0 || i32Len >= (IMG_INT32)ui32Space;
}

/*************************************************************************/ /*!
@Function       PVRSRVReleasePrintf
@Description    To output an important message to the user in release builds
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void PVRSRVReleasePrintf(const IMG_CHAR *pszFormat, ...)
{
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;
	IMG_INT32  result;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(vaArgs, pszFormat);

	GetBufferLock(&ulLockFlags);

	result = snprintf(pszBuf, (ui32BufSiz - 2), "PVR_K:  %u: ", current->pid);
	PVR_ASSERT(result>0);
	ui32BufSiz -= result;

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_ERR "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_ERR "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);
	va_end(vaArgs);
}

#if defined(PVRSRV_NEED_PVR_TRACE)

/*************************************************************************/ /*!
@Function       PVRTrace
@Description    To output a debug message to the user
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void PVRSRVTrace(const IMG_CHAR *pszFormat, ...)
{
	va_list VArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;
	IMG_INT32  result;

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(VArgs, pszFormat);

	GetBufferLock(&ulLockFlags);

	result = snprintf(pszBuf, (ui32BufSiz - 2), "PVR: %u: ", current->pid);
	PVR_ASSERT(result>0);
	ui32BufSiz -= result;

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs))
	{
		printk(KERN_ERR "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		printk(KERN_ERR "%s\n", pszBuf);
	}

	ReleaseBufferLock(ulLockFlags);

	va_end(VArgs);
}

#endif /* defined(PVRSRV_NEED_PVR_TRACE) */

#if defined(PVRSRV_NEED_PVR_DPF)

/*
 * Append a string to a buffer using formatted conversion.
 * The function takes a variable number of arguments, calling
 * VBAppend to do the actual work.
 */
__printf(3, 4)
static IMG_BOOL BAppend(IMG_CHAR *pszBuf, IMG_UINT32 ui32BufSiz, const IMG_CHAR *pszFormat, ...)
{
	va_list VArgs;
	IMG_BOOL bTrunc;

	va_start (VArgs, pszFormat);

	bTrunc = VBAppend(pszBuf, ui32BufSiz, pszFormat, VArgs);

	va_end (VArgs);

	return bTrunc;
}

/*************************************************************************/ /*!
@Function       PVRSRVDebugPrintf
@Description    To output a debug message to the user
@Input          uDebugLevel The current debug level
@Input          pszFile     The source file generating the message
@Input          uLine       The line of the source file
@Input          pszFormat   The message format string
@Input          ...         Zero or more arguments for use by the format string
*/ /**************************************************************************/
void PVRSRVDebugPrintf(IMG_UINT32 ui32DebugLevel,
			   const IMG_CHAR *pszFullFileName,
			   IMG_UINT32 ui32Line,
			   const IMG_CHAR *pszFormat,
			   ...)
{
	const IMG_CHAR *pszFileName = pszFullFileName;
	IMG_CHAR *pszLeafName;
	va_list vaArgs;
	unsigned long ulLockFlags = 0;
	IMG_CHAR *pszBuf;
	IMG_UINT32 ui32BufSiz;

	if (!(gPVRDebugLevel & ui32DebugLevel))
	{
		return;
	}

	SelectBuffer(&pszBuf, &ui32BufSiz);

	va_start(vaArgs, pszFormat);

	GetBufferLock(&ulLockFlags);

	switch (ui32DebugLevel)
	{
		case DBGPRIV_FATAL:
		{
			strncpy(pszBuf, "PVR_K:(Fatal): ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_ERROR:
		{
			strncpy(pszBuf, "PVR_K:(Error): ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_WARNING:
		{
			strncpy(pszBuf, "PVR_K:(Warn):  ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_MESSAGE:
		{
			strncpy(pszBuf, "PVR_K:(Mesg):  ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_VERBOSE:
		{
			strncpy(pszBuf, "PVR_K:(Verb):  ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_DEBUG:
		{
			strncpy(pszBuf, "PVR_K:(Debug): ", (ui32BufSiz - 2));
			break;
		}
		case DBGPRIV_CALLTRACE:
		case DBGPRIV_ALLOC:
		case DBGPRIV_BUFFERED:
		default:
		{
			strncpy(pszBuf, "PVR_K: ", (ui32BufSiz - 2));
			break;
		}
	}
	pszBuf[ui32BufSiz - 1] = '\0';

	if (current->pid == task_tgid_nr(current))
	{
		(void) BAppend(pszBuf, ui32BufSiz, "%5u: ", current->pid);
	}
	else
	{
		(void) BAppend(pszBuf, ui32BufSiz, "%5u-%5u: ", task_tgid_nr(current) /* pid id of group*/, current->pid /* task id */);
	}

	if (VBAppend(pszBuf, ui32BufSiz, pszFormat, vaArgs))
	{
		printk(KERN_ERR "PVR_K:(Message Truncated): %s\n", pszBuf);
	}
	else
	{
		IMG_BOOL bTruncated = IMG_FALSE;

#if !defined(__sh__)
		pszLeafName = (IMG_CHAR *)strrchr (pszFileName, '/');

		if (pszLeafName)
		{
			pszFileName = pszLeafName+1;
		}
#endif /* __sh__ */

#if defined(DEBUG)
		{
			static const IMG_CHAR *lastFile;

			if (lastFile == pszFileName)
			{
				bTruncated = BAppend(pszBuf, ui32BufSiz, " [%u]", ui32Line);
			}
			else
			{
				bTruncated = BAppend(pszBuf, ui32BufSiz, " [%s:%u]", pszFileName, ui32Line);
				lastFile = pszFileName;
			}
		}
#endif

		if (bTruncated)
		{
			printk(KERN_ERR "PVR_K:(Message Truncated): %s\n", pszBuf);
		}
		else
		{
			if (ui32DebugLevel & DBGPRIV_BUFFERED)
			{
				AddToBufferCCB(pszFileName, ui32Line, pszBuf);
			}
			else
			{
				printk(KERN_ERR "%s\n", pszBuf);
			}
		}
	}

	ReleaseBufferLock(ulLockFlags);

	va_end (vaArgs);
}

#endif /* PVRSRV_NEED_PVR_DPF */


/*************************************************************************/ /*!
 Version DebugFS entry
*/ /**************************************************************************/

static void *_DebugVersionCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode,
					  va_list va)
{
	loff_t *puiCurrentPosition = va_arg(va, loff_t *);
	loff_t uiPosition = va_arg(va, loff_t);
	loff_t uiCurrentPosition = *puiCurrentPosition;

	(*puiCurrentPosition)++;

	return (uiCurrentPosition == uiPosition) ? psDevNode : NULL;
}

static void *_DebugVersionSeqStart(struct seq_file *psSeqFile,
				   loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	if (*puiPosition == 0)
	{
		return SEQ_START_TOKEN;
	}

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugVersionCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DebugVersionSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugVersionSeqNext(struct seq_file *psSeqFile,
				  void *pvData,
				  loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*puiPosition)++;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugVersionCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

#define SEQ_PRINT_VERSION_FMTSPEC "%s Version: %u.%u @ %u (%s) build options: 0x%08x %s\n"
#define STR_DEBUG   "debug"
#define STR_RELEASE "release"

static int _DebugVersionSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();

	if (pvData == SEQ_START_TOKEN)
	{
		if (psPVRSRVData->sDriverInfo.bIsNoMatch)
		{
			const BUILD_INFO *psBuildInfo;

			psBuildInfo = &psPVRSRVData->sDriverInfo.sUMBuildInfo;
			seq_printf(psSeqFile, SEQ_PRINT_VERSION_FMTSPEC,
			           "UM Driver",
				       PVRVERSION_UNPACK_MAJ(psBuildInfo->ui32BuildVersion),
				       PVRVERSION_UNPACK_MIN(psBuildInfo->ui32BuildVersion),
				       psBuildInfo->ui32BuildRevision,
				       (psBuildInfo->ui32BuildType == BUILD_TYPE_DEBUG) ? STR_DEBUG : STR_RELEASE,
				       psBuildInfo->ui32BuildOptions,
					   PVR_BUILD_DIR);

			psBuildInfo = &psPVRSRVData->sDriverInfo.sKMBuildInfo;
			seq_printf(psSeqFile, SEQ_PRINT_VERSION_FMTSPEC,
			           "KM Driver",
				       PVRVERSION_UNPACK_MAJ(psBuildInfo->ui32BuildVersion),
				       PVRVERSION_UNPACK_MIN(psBuildInfo->ui32BuildVersion),
				       psBuildInfo->ui32BuildRevision,
				       (psBuildInfo->ui32BuildType == BUILD_TYPE_DEBUG) ? STR_DEBUG : STR_RELEASE,
				       psBuildInfo->ui32BuildOptions,
					   PVR_BUILD_DIR);
		}
		else
		{
			/*
			 * bIsNoMatch is `false` in one of the following cases:
			 * - UM & KM version parameters actually match.
			 * - A comparison between UM & KM has not been made yet, because no
			 *   client ever connected.
			 *
			 * In both cases, available (KM) version info is the best output we
			 * can provide.
			 */
			seq_printf(psSeqFile, "Driver Version: %s (%s) build options: 0x%08lx %s\n",
			           PVRVERSION_STRING, PVR_BUILD_TYPE, RGX_BUILD_OPTIONS_KM, PVR_BUILD_DIR);
		}
	}
	else if (pvData != NULL)
	{
		PVRSRV_DEVICE_NODE *psDevNode = (PVRSRV_DEVICE_NODE *)pvData;
#if defined(SUPPORT_RGX)
		PVRSRV_RGXDEV_INFO *psDevInfo = psDevNode->pvDevice;
		RGXFWIF_INIT *psRGXFWInit;
		PVRSRV_ERROR eError;
#endif
		IMG_BOOL bFwVersionInfoPrinted = IMG_FALSE;

		seq_printf(psSeqFile, "\nDevice Name: %s\n", psDevNode->psDevConfig->pszName);

		if (psDevNode->psDevConfig->pszVersion)
		{
			seq_printf(psSeqFile, "Device Version: %s\n", psDevNode->psDevConfig->pszVersion);
		}

		if (psDevNode->pfnDeviceVersionString)
		{
			IMG_CHAR *pszDeviceVersionString;

			if (psDevNode->pfnDeviceVersionString(psDevNode, &pszDeviceVersionString) == PVRSRV_OK)
			{
				seq_printf(psSeqFile, "%s\n", pszDeviceVersionString);

				OSFreeMem(pszDeviceVersionString);
			}
		}
#if defined(SUPPORT_RGX)
		/* print device's firmware version info */
		if (psDevInfo->psRGXFWIfInitMemDesc != NULL)
		{
			eError = DevmemAcquireCpuVirtAddr(psDevInfo->psRGXFWIfInitMemDesc, (void**)&psRGXFWInit);
			if (eError == PVRSRV_OK)
			{
				if (psRGXFWInit->sRGXCompChecks.bUpdated)
				{
					const RGXFWIF_COMPCHECKS *psRGXCompChecks = &psRGXFWInit->sRGXCompChecks;

					seq_printf(psSeqFile, SEQ_PRINT_VERSION_FMTSPEC,
							   "Firmware",
							   PVRVERSION_UNPACK_MAJ(psRGXCompChecks->ui32DDKVersion),
							   PVRVERSION_UNPACK_MIN(psRGXCompChecks->ui32DDKVersion),
							   psRGXCompChecks->ui32DDKBuild,
							   ((psRGXCompChecks->ui32BuildOptions & OPTIONS_DEBUG_MASK) ?
								   STR_DEBUG : STR_RELEASE),
							   psRGXCompChecks->ui32BuildOptions,
							   PVR_BUILD_DIR);
					bFwVersionInfoPrinted = IMG_TRUE;
				}
				DevmemReleaseCpuVirtAddr(psDevInfo->psRGXFWIfInitMemDesc);
			}
			else
			{
				PVR_DPF((PVR_DBG_ERROR,
						 "%s: Error acquiring CPU virtual address of FWInitMemDesc",
						 __func__));
			}
		}
#endif

		if (!bFwVersionInfoPrinted)
		{
			seq_printf(psSeqFile, "Firmware Version: Info unavailable %s\n",
#if defined(NO_HARDWARE)
			           "on NoHW driver"
#else
                       "(Is INIT complete?)"
#endif
                       );
		}
	}

	return 0;
}

static struct seq_operations gsDebugVersionReadOps =
{
	.start = _DebugVersionSeqStart,
	.stop = _DebugVersionSeqStop,
	.next = _DebugVersionSeqNext,
	.show = _DebugVersionSeqShow,
};

#if defined(SUPPORT_RGX) && defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
/*************************************************************************/ /*!
 Power data DebugFS entry
*/ /**************************************************************************/

static void *_DebugPowerDataCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode,
					  va_list va)
{
	loff_t *puiCurrentPosition = va_arg(va, loff_t *);
	loff_t uiPosition = va_arg(va, loff_t);
	loff_t uiCurrentPosition = *puiCurrentPosition;

	(*puiCurrentPosition)++;

	return (uiCurrentPosition == uiPosition) ? psDevNode : NULL;
}

static void *_DebugPowerDataSeqStart(struct seq_file *psSeqFile,
									 loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 0;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugPowerDataCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DebugPowerDataSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugPowerDataSeqNext(struct seq_file *psSeqFile,
									void *pvData,
									loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 0;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*puiPosition)++;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugPowerDataCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static PVRSRV_ERROR SendPowerCounterCommand(PVRSRV_DEVICE_NODE* psDeviceNode,
											RGXFWIF_COUNTER_DUMP_REQUEST eRequestType)
{
	PVRSRV_ERROR eError;

	RGXFWIF_KCCB_CMD sCounterDumpCmd;

	sCounterDumpCmd.eCmdType = RGXFWIF_KCCB_CMD_COUNTER_DUMP;
	sCounterDumpCmd.uCmdData.sCounterDumpConfigData.eCounterDumpRequest = eRequestType;

	eError = RGXScheduleCommand(psDeviceNode->pvDevice,
				RGXFWIF_DM_GP,
				&sCounterDumpCmd,
				0,
				PDUMP_FLAGS_CONTINUOUS);

	if (eError != PVRSRV_OK)
	{
		PVR_DPF((PVR_DBG_ERROR, "SendPowerCounterCommand: RGXScheduleCommand failed. Error:%u", eError));
	}

	return eError;
}

static void *_IsDevNodeNotInitialised(PVRSRV_DEVICE_NODE *psDeviceNode)
{
	return psDeviceNode->eDevState == PVRSRV_DEVICE_STATE_ACTIVE ? NULL : psDeviceNode;
}

static void _SendPowerCounterCommand(PVRSRV_DEVICE_NODE* psDeviceNode,
									 va_list va)
{
	PVRSRV_RGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;
	OSLockAcquire(psDevInfo->hCounterDumpingLock);

	SendPowerCounterCommand(psDeviceNode, va_arg(va, RGXFWIF_COUNTER_DUMP_REQUEST));

	OSLockRelease(psDevInfo->hCounterDumpingLock);
}

static int _DebugPowerDataSeqShow(struct seq_file *psSeqFile, void *pvData)
{

	PVRSRV_ERROR eError = PVRSRV_OK;

	if (pvData != NULL)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
		PVRSRV_RGXDEV_INFO 	*psDevInfo = psDeviceNode->pvDevice;

		if (psDeviceNode->eDevState != PVRSRV_DEVICE_STATE_ACTIVE)
		{
			PVR_DPF((PVR_DBG_ERROR, "Not all device nodes were initialised when power counter data was requested!"));
			return -EIO;
		}

		OSLockAcquire(psDevInfo->hCounterDumpingLock);

		eError = SendPowerCounterCommand(psDeviceNode, RGXFWIF_PWR_COUNTER_DUMP_SAMPLE);

		if (eError != PVRSRV_OK)
		{
			return -EIO;
		}

		/* Create update command to notify the host that the copy is finished. */
		{
			PVRSRV_CLIENT_SYNC_PRIM* psCopySyncPrim;
			RGXFWIF_DEV_VIRTADDR sSyncFWAddr;
			RGXFWIF_KCCB_CMD sSyncCmd;
			eError = SyncPrimAlloc(psDeviceNode->hSyncPrimContext,
								&psCopySyncPrim,
								"power counter dump sync prim");

			SyncPrimSet(psCopySyncPrim, 0);

			SyncPrimGetFirmwareAddr(psCopySyncPrim, &sSyncFWAddr.ui32Addr);

			sSyncCmd.eCmdType = RGXFWIF_KCCB_CMD_SYNC;
			sSyncCmd.uCmdData.sSyncData.sSyncObjDevVAddr = sSyncFWAddr;
			sSyncCmd.uCmdData.sSyncData.uiUpdateVal = 1;

			eError = RGXScheduleCommand(psDeviceNode->pvDevice,
						RGXFWIF_DM_GP,
						&sSyncCmd,
						0,
						PDUMP_FLAGS_CONTINUOUS);

			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "_DebugPowerDataSeqShow: RGXScheduleCommand failed. Error:%u", eError));
				OSLockRelease(psDevInfo->hCounterDumpingLock);
				return -EIO;
			}

			eError = PVRSRVWaitForValueKM(psCopySyncPrim->pui32LinAddr, 1, 0xffffffff);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "_DebugPowerDataSeqShow: PVRSRVWaitForValueKM failed. Error:%u", eError));
				OSLockRelease(psDevInfo->hCounterDumpingLock);
				return -EIO;
			}

			eError = SyncPrimFree(psCopySyncPrim);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR, "_DebugPowerDataSeqShow: SyncPrimFree failed. Error:%u", eError));
				OSLockRelease(psDevInfo->hCounterDumpingLock);
				return -EIO;
			}
		}

		/* Read back the buffer */
		{
			IMG_UINT32* pui32PowerBuffer;
			IMG_UINT32 ui32NumOfRegs, ui32SamplePeriod;
			IMG_UINT32 i,j;

			eError = DevmemAcquireCpuVirtAddr(psDevInfo->psCounterBufferMemDesc, (void**)&pui32PowerBuffer);
			if (eError != PVRSRV_OK)
			{
				PVR_DPF((PVR_DBG_ERROR,"_DebugPowerDataSeqShow: Failed to acquire buffer memory mapping (%u)", eError));
				OSLockRelease(psDevInfo->hCounterDumpingLock);
				return -EIO;
			}

			ui32NumOfRegs = *pui32PowerBuffer++;
			ui32SamplePeriod = *pui32PowerBuffer++;

			if (ui32NumOfRegs)
			{
				seq_printf(psSeqFile, "Power counter data for device id: %d\n", psDeviceNode->sDevId.i32UMIdentifier);
				seq_printf(psSeqFile, "Sample period: 0x%08x\n", ui32SamplePeriod);

				for (i = 0; i < ui32NumOfRegs; i++)
				{
					IMG_UINT32 ui32High, ui32Low;
					IMG_UINT32 ui32RegOffset = *pui32PowerBuffer++;
					IMG_UINT32 ui32NumOfInstances = *pui32PowerBuffer++;

					PVR_ASSERT(ui32NumOfInstances);

					seq_printf(psSeqFile, "0x%08x:", ui32RegOffset);

					for (j = 0; j < ui32NumOfInstances; j++)
					{
						ui32Low = *pui32PowerBuffer++;
						ui32High = *pui32PowerBuffer++;

						seq_printf(psSeqFile, " 0x%016llx", (IMG_UINT64)ui32Low | (IMG_UINT64)ui32High << 32);
					}

					seq_printf(psSeqFile, "\n");
				}
			}

			DevmemReleaseCpuVirtAddr(psDevInfo->psCounterBufferMemDesc);
		}

		OSLockRelease(psDevInfo->hCounterDumpingLock);
	}

	return eError;
}

static IMG_INT PowerDataSet(const char __user *pcBuffer,
							 size_t uiCount,
							 loff_t *puiPosition,
							 void *pvData)
{
	IMG_CHAR acDataBuffer[2];
	PVRSRV_DATA* psPVRSRVData = (PVRSRV_DATA*) pvData;

	if (puiPosition == NULL || *puiPosition != 0)
	{
		return -EIO;
	}

	if (uiCount == 0 || uiCount > ARRAY_SIZE(acDataBuffer))
	{
		return -EINVAL;
	}

	if (pvr_copy_from_user(acDataBuffer, pcBuffer, uiCount))
	{
		return -EINVAL;
	}

	if (acDataBuffer[uiCount - 1] != '\n')
	{
		return -EINVAL;
	}

	if (List_PVRSRV_DEVICE_NODE_Any(psPVRSRVData->psDeviceNodeList, _IsDevNodeNotInitialised))
	{
		PVR_DPF((PVR_DBG_ERROR, "Not all device nodes were initialised when power counter data was requested!"));
		return -EIO;
	}

	if ((acDataBuffer[0] == '1') && uiCount == 2)
	{
		List_PVRSRV_DEVICE_NODE_ForEach_va(psPVRSRVData->psDeviceNodeList,
										   _SendPowerCounterCommand,
										   RGXFWIF_PWR_COUNTER_DUMP_START);

	}
	else if ((acDataBuffer[0] == '0') && uiCount == 2)
	{

		List_PVRSRV_DEVICE_NODE_ForEach_va(psPVRSRVData->psDeviceNodeList,
										   _SendPowerCounterCommand,
										   RGXFWIF_PWR_COUNTER_DUMP_STOP);
	}
	else
	{

		return -EINVAL;
	}

	*puiPosition += uiCount;
	return uiCount;
}

static struct seq_operations gsDebugPowerDataReadOps =
{
	.start = _DebugPowerDataSeqStart,
	.stop =  _DebugPowerDataSeqStop,
	.next =  _DebugPowerDataSeqNext,
	.show =  _DebugPowerDataSeqShow,
};

#endif /* SUPPORT_RGX && SUPPORT_POWER_SAMPLING_VIA_DEBUGFS*/
/*************************************************************************/ /*!
 Status DebugFS entry
*/ /**************************************************************************/

static void *_DebugStatusCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode,
										 va_list va)
{
	loff_t *puiCurrentPosition = va_arg(va, loff_t *);
	loff_t uiPosition = va_arg(va, loff_t);
	loff_t uiCurrentPosition = *puiCurrentPosition;

	(*puiCurrentPosition)++;

	return (uiCurrentPosition == uiPosition) ? psDevNode : NULL;
}

static void *_DebugStatusSeqStart(struct seq_file *psSeqFile,
								  loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	if (*puiPosition == 0)
	{
		return SEQ_START_TOKEN;
	}

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugStatusCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DebugStatusSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugStatusSeqNext(struct seq_file *psSeqFile,
								 void *pvData,
								 loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*puiPosition)++;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugStatusCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static int _DebugStatusSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	if (pvData == SEQ_START_TOKEN)
	{
		PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;

		if (psPVRSRVData != NULL)
		{
			switch (psPVRSRVData->eServicesState)
			{
				case PVRSRV_SERVICES_STATE_OK:
					seq_printf(psSeqFile, "Driver Status:   OK\n");
					break;
				case PVRSRV_SERVICES_STATE_BAD:
					seq_printf(psSeqFile, "Driver Status:   BAD\n");
					break;
				case PVRSRV_SERVICES_STATE_UNDEFINED:
					seq_printf(psSeqFile, "Driver Status:   UNDEFINED\n");
					break;
				default:
					seq_printf(psSeqFile, "Driver Status:   UNKNOWN (%d)\n", psPVRSRVData->eServicesState);
					break;
			}
		}
	}
	else if (pvData != NULL)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;
		IMG_CHAR           *pszStatus = "";
		IMG_CHAR           *pszReason = "";
		PVRSRV_DEVICE_HEALTH_STATUS eHealthStatus;
		PVRSRV_DEVICE_HEALTH_REASON eHealthReason;

		/* Update the health status now if possible... */
		if (psDeviceNode->pfnUpdateHealthStatus)
		{
			psDeviceNode->pfnUpdateHealthStatus(psDeviceNode, IMG_FALSE);
		}
		eHealthStatus = OSAtomicRead(&psDeviceNode->eHealthStatus);
		eHealthReason = OSAtomicRead(&psDeviceNode->eHealthReason);

		switch (eHealthStatus)
		{
			case PVRSRV_DEVICE_HEALTH_STATUS_OK:  pszStatus = "OK";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_NOT_RESPONDING:  pszStatus = "NOT RESPONDING";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_DEAD:  pszStatus = "DEAD";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_FAULT:  pszStatus = "FAULT";  break;
			case PVRSRV_DEVICE_HEALTH_STATUS_UNDEFINED:  pszStatus = "UNDEFINED";  break;
			default:  pszStatus = "UNKNOWN";  break;
		}

		switch (eHealthReason)
		{
			case PVRSRV_DEVICE_HEALTH_REASON_NONE:  pszReason = "";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_ASSERTED:  pszReason = " (Asserted)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_POLL_FAILING:  pszReason = " (Poll failure)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_TIMEOUTS:  pszReason = " (Global Event Object timeouts rising)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_QUEUE_CORRUPT:  pszReason = " (KCCB offset invalid)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_QUEUE_STALLED:  pszReason = " (KCCB stalled)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_IDLING:  pszReason = " (Idling)";  break;
			case PVRSRV_DEVICE_HEALTH_REASON_RESTARTING:  pszReason = " (Restarting)";  break;
			default:  pszReason = " (Unknown reason)";  break;
		}

		seq_printf(psSeqFile, "Firmware Status: %s%s\n", pszStatus, pszReason);

		if (PVRSRV_VZ_MODE_IS(DRIVER_MODE_GUEST))
		{
			/*
			 * Guest drivers do not support the following functionality:
			 *	- Perform actual on-chip fw tracing.
			 *	- Collect actual on-chip GPU utilization stats.
			 *	- Perform actual on-chip GPU power/dvfs management.
			 *	- As a result no more information can be provided.
			 */
			return 0;
		}

		/* Write other useful stats to aid the test cycle... */
		if (psDeviceNode->pvDevice != NULL)
		{
#if defined(SUPPORT_RGX)
			PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;
			RGXFWIF_TRACEBUF *psRGXFWIfTraceBufCtl = psDevInfo->psRGXFWIfTraceBuf;

			/* Calculate the number of HWR events in total across all the DMs... */
			if (psRGXFWIfTraceBufCtl != NULL)
			{
				IMG_UINT32 ui32HWREventCount = 0;
				IMG_UINT32 ui32CRREventCount = 0;
				IMG_UINT32 ui32DMIndex;

				for (ui32DMIndex = 0; ui32DMIndex < RGXFWIF_DM_MAX; ui32DMIndex++)
				{
					ui32HWREventCount += psRGXFWIfTraceBufCtl->aui32HwrDmLockedUpCount[ui32DMIndex];
					ui32CRREventCount += psRGXFWIfTraceBufCtl->aui32HwrDmOverranCount[ui32DMIndex];
				}

				seq_printf(psSeqFile, "HWR Event Count: %d\n", ui32HWREventCount);
				seq_printf(psSeqFile, "CRR Event Count: %d\n", ui32CRREventCount);
				seq_printf(psSeqFile, "FWF Event Count: %d\n", psRGXFWIfTraceBufCtl->ui32FWFaults);
			}

			/* Write the number of APM events... */
			seq_printf(psSeqFile, "APM Event Count: %d\n", psDevInfo->ui32ActivePMReqTotal);

#if defined(PVRSRV_STALLED_CCB_ACTION)
			if ((psRGXFWIfTraceBufCtl != NULL) &&
				(psRGXFWIfTraceBufCtl->ui32TracebufFlags & RGXFWIF_TRACEBUFCFG_SLR_LOG))
			{
				/* Write the number of Sync Lockup Recovery (SLR) events... */
				seq_printf(psSeqFile, "SLR Event Count: %d\n", psRGXFWIfTraceBufCtl->ui32ForcedUpdatesRequested);
			}
#endif

			/* Write the current GPU Utilisation values... */
			if (psDevInfo->pfnGetGpuUtilStats &&
				eHealthStatus == PVRSRV_DEVICE_HEALTH_STATUS_OK)
			{
				RGXFWIF_GPU_UTIL_STATS sGpuUtilStats;
				PVRSRV_ERROR eError = PVRSRV_OK;

				eError = psDevInfo->pfnGetGpuUtilStats(psDeviceNode,
													   ghGpuUtilUserDebugFS,
													   &sGpuUtilStats);

				if ((eError == PVRSRV_OK) &&
					((IMG_UINT32)sGpuUtilStats.ui64GpuStatCumulative))
				{
					IMG_UINT64 util;
					IMG_UINT32 rem;

					util = 100 * (sGpuUtilStats.ui64GpuStatActiveHigh +
								  sGpuUtilStats.ui64GpuStatActiveLow);
					util = OSDivide64(util, (IMG_UINT32)sGpuUtilStats.ui64GpuStatCumulative, &rem);

					seq_printf(psSeqFile, "GPU Utilisation: %u%%\n", (IMG_UINT32)util);
				}
				else
				{
					seq_printf(psSeqFile, "GPU Utilisation: -\n");
				}
			}
#endif
		}
	}

	return 0;
}

static ssize_t DebugStatusSet(const char __user *pcBuffer,
							  size_t uiCount,
							  loff_t *puiPosition,
							  void *pvData)
{
	IMG_CHAR acDataBuffer[6];

	if (puiPosition == NULL || *puiPosition != 0)
	{
		return -EIO;
	}

	if (uiCount == 0 || uiCount > ARRAY_SIZE(acDataBuffer))
	{
		return -EINVAL;
	}

	if (pvr_copy_from_user(acDataBuffer, pcBuffer, uiCount))
	{
		return -EINVAL;
	}

	if (acDataBuffer[uiCount - 1] != '\n')
	{
		return -EINVAL;
	}

	if (((acDataBuffer[0] == 'k') || ((acDataBuffer[0] == 'K'))) && uiCount == 2)
	{
		PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
		psPVRSRVData->eServicesState = PVRSRV_SERVICES_STATE_BAD;
	}
	else
	{
		return -EINVAL;
	}

	*puiPosition += uiCount;
	return uiCount;
}

static struct seq_operations gsDebugStatusReadOps =
{
	.start = _DebugStatusSeqStart,
	.stop = _DebugStatusSeqStop,
	.next = _DebugStatusSeqNext,
	.show = _DebugStatusSeqShow,
};

/*************************************************************************/ /*!
 Dump Debug DebugFS entry
*/ /**************************************************************************/

static void *_DebugDumpDebugCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode, va_list va)
{
	loff_t *puiCurrentPosition = va_arg(va, loff_t *);
	loff_t uiPosition = va_arg(va, loff_t);
	loff_t uiCurrentPosition = *puiCurrentPosition;

	(*puiCurrentPosition)++;

	return (uiCurrentPosition == uiPosition) ? psDevNode : NULL;
}

static void *_DebugDumpDebugSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	if (*puiPosition == 0)
	{
		return SEQ_START_TOKEN;
	}

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugDumpDebugCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DebugDumpDebugSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugDumpDebugSeqNext(struct seq_file *psSeqFile,
									void *pvData,
									loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*puiPosition)++;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugDumpDebugCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DumpDebugSeqPrintf(void *pvDumpDebugFile,
				const IMG_CHAR *pszFormat, ...)
{
	struct seq_file *psSeqFile = (struct seq_file *)pvDumpDebugFile;
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	va_list ArgList;

	va_start(ArgList, pszFormat);
	vsnprintf(szBuffer, PVR_MAX_DEBUG_MESSAGE_LEN, pszFormat, ArgList);
	va_end(ArgList);
	seq_printf(psSeqFile, "%s\n", szBuffer);
}

static int _DebugDumpDebugSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	if (pvData != NULL  &&  pvData != SEQ_START_TOKEN)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;

		if (psDeviceNode->pvDevice != NULL)
		{
			PVRSRVDebugRequest(psDeviceNode, DEBUG_REQUEST_VERBOSITY_MAX,
						_DumpDebugSeqPrintf, psSeqFile);
		}
	}

	return 0;
}

static struct seq_operations gsDumpDebugReadOps =
{
	.start = _DebugDumpDebugSeqStart,
	.stop  = _DebugDumpDebugSeqStop,
	.next  = _DebugDumpDebugSeqNext,
	.show  = _DebugDumpDebugSeqShow,
};

#if defined(SUPPORT_RGX)
/*************************************************************************/ /*!
 Firmware Trace DebugFS entry
*/ /**************************************************************************/
static void *_DebugFWTraceCompare_AnyVaCb(PVRSRV_DEVICE_NODE *psDevNode, va_list va)
{
	loff_t *puiCurrentPosition = va_arg(va, loff_t *);
	loff_t uiPosition = va_arg(va, loff_t);
	loff_t uiCurrentPosition = *puiCurrentPosition;

	(*puiCurrentPosition)++;

	return (uiCurrentPosition == uiPosition) ? psDevNode : NULL;
}

static void *_DebugFWTraceSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	if (*puiPosition == 0)
	{
		return SEQ_START_TOKEN;
	}

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugFWTraceCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _DebugFWTraceSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *_DebugFWTraceSeqNext(struct seq_file *psSeqFile,
								  void *pvData,
								  loff_t *puiPosition)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;
	loff_t uiCurrentPosition = 1;

	PVR_UNREFERENCED_PARAMETER(pvData);

	(*puiPosition)++;

	return List_PVRSRV_DEVICE_NODE_Any_va(psPVRSRVData->psDeviceNodeList,
										  _DebugFWTraceCompare_AnyVaCb,
										  &uiCurrentPosition,
										  *puiPosition);
}

static void _FWTraceSeqPrintf(void *pvDumpDebugFile,
				const IMG_CHAR *pszFormat, ...)
{
	struct seq_file *psSeqFile = (struct seq_file *)pvDumpDebugFile;
	IMG_CHAR szBuffer[PVR_MAX_DEBUG_MESSAGE_LEN];
	va_list ArgList;

	va_start(ArgList, pszFormat);
	vsnprintf(szBuffer, PVR_MAX_DEBUG_MESSAGE_LEN, pszFormat, ArgList);
	va_end(ArgList);
	seq_printf(psSeqFile, "%s\n", szBuffer);
}

static int _DebugFWTraceSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	if (pvData != NULL  &&  pvData != SEQ_START_TOKEN)
	{
		PVRSRV_DEVICE_NODE *psDeviceNode = (PVRSRV_DEVICE_NODE *)pvData;

		if (psDeviceNode->pvDevice != NULL)
		{
			PVRSRV_RGXDEV_INFO *psDevInfo = psDeviceNode->pvDevice;

			RGXDumpFirmwareTrace(_FWTraceSeqPrintf, psSeqFile, psDevInfo);
		}
	}

	return 0;
}

static struct seq_operations gsFWTraceReadOps =
{
	.start = _DebugFWTraceSeqStart,
	.stop  = _DebugFWTraceSeqStop,
	.next  = _DebugFWTraceSeqNext,
	.show  = _DebugFWTraceSeqShow,
};

#if defined(SUPPORT_FIRMWARE_GCOV)

static PVRSRV_RGXDEV_INFO *getPsDevInfo(struct seq_file *psSeqFile)
{
	PVRSRV_DATA *psPVRSRVData = (PVRSRV_DATA *)psSeqFile->private;

	if (psPVRSRVData != NULL)
	{
		if (psPVRSRVData->psDeviceNodeList != NULL)
		{
			PVRSRV_RGXDEV_INFO *psDevInfo = (PVRSRV_RGXDEV_INFO*)psPVRSRVData->psDeviceNodeList->pvDevice;
			return psDevInfo;
		}
	}
	return NULL;
}

static void *_FirmwareGcovSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = getPsDevInfo(psSeqFile);

	if (psDevInfo != NULL)
	{
		if (psDevInfo->psFirmwareGcovBufferMemDesc != NULL)
		{
			void *pvCpuVirtAddr;
			DevmemAcquireCpuVirtAddr(psDevInfo->psFirmwareGcovBufferMemDesc, &pvCpuVirtAddr);
			return *puiPosition ? NULL : pvCpuVirtAddr;
		}
	}

	return NULL;
}

static void _FirmwareGcovSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = getPsDevInfo(psSeqFile);

	PVR_UNREFERENCED_PARAMETER(pvData);

	if (psDevInfo != NULL)
	{
		if (psDevInfo->psFirmwareGcovBufferMemDesc != NULL)
		{
			DevmemReleaseCpuVirtAddr(psDevInfo->psFirmwareGcovBufferMemDesc);
		}
	}
}

static void *_FirmwareGcovSeqNext(struct seq_file *psSeqFile,
								  void *pvData,
								  loff_t *puiPosition)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(puiPosition);
	return NULL;
}

static int _FirmwareGcovSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	PVRSRV_RGXDEV_INFO *psDevInfo = getPsDevInfo(psSeqFile);

	if (psDevInfo != NULL)
	{
		seq_write(psSeqFile, pvData, psDevInfo->ui32FirmwareGcovSize);
	}
	return 0;
}

static struct seq_operations gsFirmwareGcovReadOps =
{
	.start = _FirmwareGcovSeqStart,
	.stop  = _FirmwareGcovSeqStop,
	.next  = _FirmwareGcovSeqNext,
	.show  = _FirmwareGcovSeqShow,
};

#endif /* defined(SUPPORT_FIRMWARE_GCOV) */


#endif /* defined(SUPPORT_RGX) */
/*************************************************************************/ /*!
 Debug level DebugFS entry
*/ /**************************************************************************/

#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
static void *DebugLevelSeqStart(struct seq_file *psSeqFile, loff_t *puiPosition)
{
	if (*puiPosition == 0)
	{
		return psSeqFile->private;
	}

	return NULL;
}

static void DebugLevelSeqStop(struct seq_file *psSeqFile, void *pvData)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
}

static void *DebugLevelSeqNext(struct seq_file *psSeqFile,
							   void *pvData,
							   loff_t *puiPosition)
{
	PVR_UNREFERENCED_PARAMETER(psSeqFile);
	PVR_UNREFERENCED_PARAMETER(pvData);
	PVR_UNREFERENCED_PARAMETER(puiPosition);

	return NULL;
}

static int DebugLevelSeqShow(struct seq_file *psSeqFile, void *pvData)
{
	if (pvData != NULL)
	{
		IMG_UINT32 uiDebugLevel = *((IMG_UINT32 *)pvData);

		seq_printf(psSeqFile, "%u\n", uiDebugLevel);

		return 0;
	}

	return -EINVAL;
}

static struct seq_operations gsDebugLevelReadOps =
{
	.start = DebugLevelSeqStart,
	.stop = DebugLevelSeqStop,
	.next = DebugLevelSeqNext,
	.show = DebugLevelSeqShow,
};


static IMG_INT DebugLevelSet(const char __user *pcBuffer,
							 size_t uiCount,
							 loff_t *puiPosition,
							 void *pvData)
{
	IMG_UINT32 *uiDebugLevel = (IMG_UINT32 *)pvData;
	IMG_CHAR acDataBuffer[6];

	if (puiPosition == NULL || *puiPosition != 0)
	{
		return -EIO;
	}

	if (uiCount == 0 || uiCount > ARRAY_SIZE(acDataBuffer))
	{
		return -EINVAL;
	}

	if (pvr_copy_from_user(acDataBuffer, pcBuffer, uiCount))
	{
		return -EINVAL;
	}

	if (acDataBuffer[uiCount - 1] != '\n')
	{
		return -EINVAL;
	}

	if (sscanf(acDataBuffer, "%u", &gPVRDebugLevel) == 0)
	{
		return -EINVAL;
	}

	/* As this is Linux the next line uses a GCC builtin function */
	(*uiDebugLevel) &= (1 << __builtin_ffsl(DBGPRIV_LAST)) - 1;

	*puiPosition += uiCount;
	return uiCount;
}
#endif /* defined(DEBUG) */

static PPVR_DEBUGFS_ENTRY_DATA gpsVersionDebugFSEntry;

static PPVR_DEBUGFS_ENTRY_DATA gpsStatusDebugFSEntry;
static PPVR_DEBUGFS_ENTRY_DATA gpsDumpDebugDebugFSEntry;

#if defined(SUPPORT_RGX)
static PPVR_DEBUGFS_ENTRY_DATA gpsFWTraceDebugFSEntry;
#if defined(SUPPORT_FIRMWARE_GCOV)
static PPVR_DEBUGFS_ENTRY_DATA gpsFirmwareGcovDebugFSEntry;
#endif
#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
static PPVR_DEBUGFS_ENTRY_DATA gpsPowerDataDebugFSEntry;
#endif
#endif

#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
static PPVR_DEBUGFS_ENTRY_DATA gpsDebugLevelDebugFSEntry;
#endif

int PVRDebugCreateDebugFSEntries(void)
{
	PVRSRV_DATA *psPVRSRVData = PVRSRVGetPVRSRVData();
	int iResult;

	PVR_ASSERT(psPVRSRVData != NULL);

	/*
	 * The DebugFS entries are designed to work in a single device system but
	 * this function will be called multiple times in a multi-device system.
	 * Return an error in this case.
	 */
	if (gpsVersionDebugFSEntry)
	{
		return -EEXIST;
	}

#if defined(SUPPORT_RGX) && !defined(NO_HARDWARE)
	if (SORgxGpuUtilStatsRegister(&ghGpuUtilUserDebugFS) != PVRSRV_OK)
	{
		return -ENOMEM;
	}
#endif

	iResult = PVRDebugFSCreateFile("version",
									NULL,
									&gsDebugVersionReadOps,
									NULL,
									NULL,
									psPVRSRVData,
									&gpsVersionDebugFSEntry);
	if (iResult != 0)
	{
		goto PVRDebugCreateDebugFSEntriesErrorExit;
	}

	iResult = PVRDebugFSCreateFile("status",
									NULL,
									&gsDebugStatusReadOps,
									(PVRSRV_ENTRY_WRITE_FUNC *)DebugStatusSet,
									NULL,
									psPVRSRVData,
									&gpsStatusDebugFSEntry);
	if (iResult != 0)
	{
		goto PVRDebugCreateDebugFSEntriesErrorExit;
	}

	iResult = PVRDebugFSCreateFile("debug_dump",
									NULL,
									&gsDumpDebugReadOps,
									NULL,
									NULL,
									psPVRSRVData,
									&gpsDumpDebugDebugFSEntry);
	if (iResult != 0)
	{
		goto PVRDebugCreateDebugFSEntriesErrorExit;
	}

#if defined(SUPPORT_RGX)
	if (! PVRSRV_VZ_MODE_IS(DRIVER_MODE_GUEST))
	{
		iResult = PVRDebugFSCreateFile("firmware_trace",
										NULL,
										&gsFWTraceReadOps,
										NULL,
										NULL,
										psPVRSRVData,
										&gpsFWTraceDebugFSEntry);
		if (iResult != 0)
		{
			goto PVRDebugCreateDebugFSEntriesErrorExit;
		}
	}

#if defined(SUPPORT_FIRMWARE_GCOV)
	{

		iResult = PVRDebugFSCreateFile("firmware_gcov",
										NULL,
										&gsFirmwareGcovReadOps,
										NULL,
										NULL,
										psPVRSRVData,
										&gpsFirmwareGcovDebugFSEntry);

		if (iResult != 0)
		{
			goto PVRDebugCreateDebugFSEntriesErrorExit;
		}
	}
#endif

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	iResult = PVRDebugFSCreateFile("power_data",
									NULL,
									&gsDebugPowerDataReadOps,
									(PVRSRV_ENTRY_WRITE_FUNC *)PowerDataSet,
									NULL,
									psPVRSRVData,
									&gpsPowerDataDebugFSEntry);
	if (iResult != 0)
	{
		goto PVRDebugCreateDebugFSEntriesErrorExit;
	}
#endif
#endif

#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
	iResult = PVRDebugFSCreateFile("debug_level",
									NULL,
									&gsDebugLevelReadOps,
									(PVRSRV_ENTRY_WRITE_FUNC *)DebugLevelSet,
									NULL,
									&gPVRDebugLevel,
									&gpsDebugLevelDebugFSEntry);
	if (iResult != 0)
	{
		goto PVRDebugCreateDebugFSEntriesErrorExit;
	}
#endif

	return 0;

PVRDebugCreateDebugFSEntriesErrorExit:

	PVRDebugRemoveDebugFSEntries();

	return iResult;
}

void PVRDebugRemoveDebugFSEntries(void)
{
#if defined(SUPPORT_RGX) && !defined(NO_HARDWARE)
	if (ghGpuUtilUserDebugFS != NULL)
	{
		SORgxGpuUtilStatsUnregister(ghGpuUtilUserDebugFS);
		ghGpuUtilUserDebugFS = NULL;
	}
#endif

#if defined(DEBUG) || defined(PVR_DPF_ADHOC_DEBUG_ON)
	if (gpsDebugLevelDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsDebugLevelDebugFSEntry);
	}
#endif

#if defined(SUPPORT_RGX)
	if (gpsFWTraceDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsFWTraceDebugFSEntry);
	}

#if defined(SUPPORT_FIRMWARE_GCOV)
	if (gpsFirmwareGcovDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsFirmwareGcovDebugFSEntry);
	}
#endif

#if defined(SUPPORT_POWER_SAMPLING_VIA_DEBUGFS)
	if (gpsPowerDataDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsPowerDataDebugFSEntry);
	}
#endif

#endif /* defined(SUPPORT_RGX) */

	if (gpsDumpDebugDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsDumpDebugDebugFSEntry);
	}

	if (gpsStatusDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsStatusDebugFSEntry);
	}

	if (gpsVersionDebugFSEntry != NULL)
	{
		PVRDebugFSRemoveFile(&gpsVersionDebugFSEntry);
	}
}
