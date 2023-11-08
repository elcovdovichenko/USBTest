// Control.cpp -- IOCTL handlers for usbprobe driver
// Copyright (C) 1999, 2000 by Walter Oney
// Copyright (C) 2001 by Vyacheslav Vdovichenko
// All rights reserved

#include "stddcls.h"
#include "driver.h"
#include "ioctls.h"

NTSTATUS CacheControlRequest(PDEVICE_EXTENSION pdx, PIRP Irp, PIRP* pIrp);
VOID OnCancelPendingIoctl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS OnCompletePendingIoctl(PDEVICE_OBJECT junk, PIRP Irp, PDEVICE_EXTENSION pdx);

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchControl
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);
	ULONG info = 0;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	KdPrint((DRIVERNAME " - Irp->AssociatedIrp.SystemBuffer = 0x%x\n", Irp->AssociatedIrp.SystemBuffer));
	
	switch (code)
		{						// process request

	case IOCTL_GET_INFO:				// code == 0x800
		{						// IOCTL_GET_INFO

		KdPrint((DRIVERNAME " - IOCTL_GET_INFO = 0x%x\n", code));

		// TODO insert code here to handle this IOCTL, which uses METHOD_BUFFERED
		if (cbout != 40)
			{
			KdPrint((DRIVERNAME " - Length IOCTL_GET_INFO %d is wrong\n", cbout));
			status = STATUS_INVALID_PARAMETER;
			break;
			}

		PULONG pdwo = (PULONG)Irp->AssociatedIrp.SystemBuffer;
		pdwo[0] = (ULONG)pdx->h1pipe;
		pdwo[1] = (ULONG)pdx->h2pipe;
        info = cbout;
		break;
		}						// IOCTL_GET_INFO

	case IOCTL_1_PIPE:				// code == 0x801
		{						// IOCTL_1_PIPE

		KdPrint((DRIVERNAME " - IOCTL_1_PIPE = 0x%x\n", code));
		
		// TODO insert code here to handle this IOCTL, which uses METHOD_BUFFERED
		if (cbin != 2)
			{
			KdPrint((DRIVERNAME " - Length bulk transaction %d is wrong\n", cbin));
			status = STATUS_INVALID_PARAMETER;
			break;
			}
		
		// Exchange Lo and Hi in command WORD
		PUCHAR pw = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
		UCHAR c = *pw; *pw = *(pw+1); *(pw+1) = c;
				
		URB urb;
		UsbBuildInterruptOrBulkTransferRequest(&urb, sizeof(_URB_BULK_OR_INTERRUPT_TRANSFER),
			pdx->h1pipe, Irp->AssociatedIrp.SystemBuffer, NULL, cbin, 0, NULL);
		
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			KdPrint((DRIVERNAME " - Error %X (USBD status code %X) trying to write endpoint\n", status, urb.UrbHeader.Status));
		else
			{
			
			PUCHAR pdwo = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    		KdPrint((DRIVERNAME " - IOCTL 0x%x transaction: ", code));
			for (ULONG i = 0; i < cbin; i++)
				{
				KdPrint(("%x ",*pdwo));
				++pdwo;
				}
			KdPrint(("\n"));
		
			info = cbin;
			}

		break;
		}						// IOCTL_1_PIPE

	case IOCTL_2_PIPE:				// code == 0x802
		{						// IOCTL_2_PIPE

		KdPrint((DRIVERNAME " - IOCTL_2_PIPE = 0x%x\n", code));
		
		// TODO This is an asynchronous IOCTL using METHOD_BUFFERED. You should have
		// a PIRP member of the device extension reserved to point to the currently
		// outstanding IRP of this type. Follow this template for handling this
		// operation:
		//
		// if (<request parameters invalid in some way>)
		//	  status = STATUS_INVALID_PARAMETER;
		// else
		//    status = CacheControlRequest(pdx, Irp, &pdx->??);
		
		if (cbout != 32)
			{
			KdPrint((DRIVERNAME " - Length interrupt transaction %d is wrong\n", cbout));
			status = STATUS_INVALID_PARAMETER;
			break;
			}
		
		URB urb;
		UsbBuildInterruptOrBulkTransferRequest(&urb, sizeof(_URB_BULK_OR_INTERRUPT_TRANSFER),
			pdx->h2pipe, Irp->AssociatedIrp.SystemBuffer, NULL, cbout, 
			USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK, NULL);
				
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			KdPrint((DRIVERNAME " - Error %X (USBD status code %X) trying to read endpoint\n", status, urb.UrbHeader.Status));
		else
			{
	
			PUCHAR pdwo = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
    		KdPrint((DRIVERNAME " - IOCTL 0x%x transaction: ", code));
			for (ULONG i = 0; i < cbout; i++)
				{
				KdPrint(("%x ",*pdwo));
				++pdwo;
				}
			KdPrint(("\n"));
		
			info = cbout;
			}

		break;
		}						// IOCTL_2PIPE

	default:
		KdPrint((DRIVERNAME " - IOCTL_???????? = 0x%x\n", code));
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;

		}						// process request

	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status == STATUS_PENDING ? status : CompleteRequest(Irp, status, info);
	}							// DispatchControl

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID AbortPendingIoctls(PDEVICE_EXTENSION pdx, NTSTATUS status)
	{							// AbortPendingIoctls
	PAGED_CODE();
	InterlockedExchange(&pdx->IoctlAbortStatus, status);
	CleanupControlRequests(pdx, status, NULL);
	}							// AbortPendingIoctls

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS CacheControlRequest(PDEVICE_EXTENSION pdx, PIRP Irp, PIRP* pIrp)
	{							// CacheControlRequest
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

	KIRQL oldirql;
	KeAcquireSpinLock(&pdx->IoctlListLock, &oldirql);

	NTSTATUS status;

	if (*pIrp)
		status = STATUS_UNSUCCESSFUL;	// something already cached here
	else if (pdx->IoctlAbortStatus)
		status = pdx->IoctlAbortStatus;	// rejecting new IRPs for some reason
	else
		{						// try to cache IRP

		// Install a cancel routine and check for this IRP having already been
		// cancelled

		IoSetCancelRoutine(Irp, OnCancelPendingIoctl);
		if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
			status = STATUS_CANCELLED;	// already cancelled

		// Put this IRP on our list of pending IOCTLs. Install a completion
		// routine to nullify the cache pointer. Note that AddDevice would have
		// failed if there were no PDO below us, so we know there's at least
		// one free stack location here.

		else
			{					// cache it
			IoMarkIrpPending(Irp);
			status = STATUS_PENDING;

			PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
			stack->Parameters.Others.Argument1 = (PVOID) pIrp;
			IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) OnCompletePendingIoctl, (PVOID) pdx, TRUE, TRUE, TRUE);
			IoSetNextIrpStackLocation(Irp);	// so our completion routine will get called
			PFILE_OBJECT fop = stack->FileObject;
			stack = IoGetCurrentIrpStackLocation(Irp);
			stack->DeviceObject = pdx->DeviceObject;	// so IoCancelIrp can give us right ptr
			stack->FileObject = fop;	// for cleanup

			*pIrp = Irp;
			InsertTailList(&pdx->PendingIoctlList, &Irp->Tail.Overlay.ListEntry);
			}					// cache it
		}						// try to cache IRP

	KeReleaseSpinLock(&pdx->IoctlListLock, oldirql);
	return status;
	}							// CacheControlRequest

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID CleanupControlRequests(PDEVICE_EXTENSION pdx, NTSTATUS status, PFILE_OBJECT fop)
	{							// CleanupControlRequests
	LIST_ENTRY cancellist;
	InitializeListHead(&cancellist);

	// Create a list of IRPs that belong to the same file object

	KIRQL oldirql;
	KeAcquireSpinLock(&pdx->IoctlListLock, &oldirql);

	PLIST_ENTRY first = &pdx->PendingIoctlList;
	PLIST_ENTRY next;

	for (next = first->Flink; next != first; )
		{						// for each queued IRP
		PIRP Irp = CONTAINING_RECORD(next, IRP, Tail.Overlay.ListEntry);
		PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

		// Follow the chain to the next IRP now (so that the next iteration of
		// the loop is properly setup whether we dequeue this IRP or not)

		PLIST_ENTRY current = next;
		next = next->Flink;

		// Skip this IRP if it's not for the same file object as the
		// current IRP_MJ_CLEANUP.

		if (fop && stack->FileObject != fop)
			continue;			// not for same file object

		// Set the CancelRoutine pointer to NULL and remove the IRP from the
		// queue.

		if (!IoSetCancelRoutine(Irp, NULL))
			continue;			// being cancelled right this instant
		RemoveEntryList(current);
		InsertTailList(&cancellist, current);
		}						// for each queued IRP

	// Release the spin lock. We're about to undertake a potentially time-consuming
	// operation that might conceivably result in a deadlock if we keep the lock.

	KeReleaseSpinLock(&pdx->IoctlListLock, oldirql);

	// Complete the selected requests.

	while (!IsListEmpty(&cancellist))
		{						// cancel selected requests
		next = RemoveHeadList(&cancellist);
		PIRP Irp = CONTAINING_RECORD(next, IRP, Tail.Overlay.ListEntry);
		Irp->IoStatus.Status = STATUS_CANCELLED;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		}						// cancel selected requests
	}							// CleanupControlRequests

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

VOID OnCancelPendingIoctl(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// OnCancelPendingIoctl
	KIRQL oldirql = Irp->CancelIrql;
	IoReleaseCancelSpinLock(DISPATCH_LEVEL);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Remove the IRP from whatever queue it's on

	KeAcquireSpinLockAtDpcLevel(&pdx->IoctlListLock);
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
	KeReleaseSpinLock(&pdx->IoctlListLock, oldirql);

	// Complete the IRP

	Irp->IoStatus.Status = STATUS_CANCELLED;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}							// OnCancelPendingIoctl

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

NTSTATUS OnCompletePendingIoctl(PDEVICE_OBJECT junk, PIRP Irp, PDEVICE_EXTENSION pdx)
	{							// OnCompletePendingIoctl
	KIRQL oldirql;
	KeAcquireSpinLock(&pdx->IoctlListLock, &oldirql);
	PIRP* pIrp = (PIRP*) IoGetCurrentIrpStackLocation(Irp)->Parameters.Others.Argument1;
	if (*pIrp == Irp)
		*pIrp = NULL;
	KeReleaseSpinLock(&pdx->IoctlListLock, oldirql);
	return STATUS_SUCCESS;
	}							// OnCompletePendingIoctl

///////////////////////////////////////////////////////////////////////////////

#pragma LOCKEDCODE

PIRP UncacheControlRequest(PDEVICE_EXTENSION pdx, PIRP* pIrp)
	{							// UncacheControlRequest
	ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
	
	KIRQL oldirql;
	KeAcquireSpinLock(&pdx->IoctlListLock, &oldirql);

	PIRP Irp = (PIRP) InterlockedExchangePointer(pIrp, NULL);

	if (Irp)
		{						// an IRP was cached

		// Clear the cancel pointer for this IRP. Since both we and the
		// completion routine use a spin lock, it cannot happen that this
		// IRP pointer is suddenly invalid but the cache pointer cell
		// wasn't already NULL.

		if (IoSetCancelRoutine(Irp, NULL))
			{
			RemoveEntryList(&Irp->Tail.Overlay.ListEntry);	// N.B.: a macro!!
			}
		else
			Irp = NULL;			// currently being cancelled
		}						// an IRP was cached

	KeReleaseSpinLock(&pdx->IoctlListLock, oldirql);

	return Irp;
	}							// UncacheControlRequest
