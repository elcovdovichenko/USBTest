// Plug and Play handlers for usbprobe driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

NTSTATUS DefaultPnpHandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleCancelRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleCancelStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryCapabilities(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleQueryStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleRemoveDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleStartDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleStopDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS HandleSurpriseRemoval(IN PDEVICE_OBJECT fdo, IN PIRP Irp);

#if DBG

static char* statenames[] = {
	"STOPPED",
	"WORKING",
	"PENDINGSTOP",
	"PENDINGREMOVE",
	"SURPRISEREMOVED",
	"REMOVED",
	};

#endif

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DispatchPnp
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	NTSTATUS status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ASSERT(stack->MajorFunction == IRP_MJ_PNP);

	static NTSTATUS (*fcntab[])(IN PDEVICE_OBJECT fdo, IN PIRP Irp) = {
		HandleStartDevice,		// IRP_MN_START_DEVICE
		HandleQueryRemove,		// IRP_MN_QUERY_REMOVE_DEVICE
		HandleRemoveDevice,		// IRP_MN_REMOVE_DEVICE
		HandleCancelRemove,		// IRP_MN_CANCEL_REMOVE_DEVICE
		HandleStopDevice,		// IRP_MN_STOP_DEVICE
		HandleQueryStop,		// IRP_MN_QUERY_STOP_DEVICE
		HandleCancelStop,		// IRP_MN_CANCEL_STOP_DEVICE
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_RELATIONS
		DefaultPnpHandler,		// IRP_MN_QUERY_INTERFACE
		HandleQueryCapabilities,// IRP_MN_QUERY_CAPABILITIES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCES
		DefaultPnpHandler,		// IRP_MN_QUERY_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// IRP_MN_QUERY_DEVICE_TEXT
		DefaultPnpHandler,		// IRP_MN_FILTER_RESOURCE_REQUIREMENTS
		DefaultPnpHandler,		// 
		DefaultPnpHandler,		// IRP_MN_READ_CONFIG
		DefaultPnpHandler,		// IRP_MN_WRITE_CONFIG
		DefaultPnpHandler,		// IRP_MN_EJECT
		DefaultPnpHandler,		// IRP_MN_SET_LOCK
		DefaultPnpHandler,		// IRP_MN_QUERY_ID
		DefaultPnpHandler,		// IRP_MN_QUERY_PNP_DEVICE_STATE
		DefaultPnpHandler,		// IRP_MN_QUERY_BUS_INFORMATION
		DefaultPnpHandler,		// IRP_MN_DEVICE_USAGE_NOTIFICATION
		HandleSurpriseRemoval,	// IRP_MN_SURPRISE_REMOVAL
		};

	ULONG fcn = stack->MinorFunction;
	if (fcn >= arraysize(fcntab))
		{						// unknown function
		status = DefaultPnpHandler(fdo, Irp); // some function we don't know about
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
		return status;
		}						// unknown function

#if DBG
	static char* fcnname[] = {
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
		};

	KdPrint((DRIVERNAME " - PNP Request (%s)\n", fcnname[fcn]));
#endif // DBG

	status = (*fcntab[fcn])(fdo, Irp);
	if (fcn != IRP_MN_REMOVE_DEVICE)
		IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
	}							// DispatchPnp

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

NTSTATUS DefaultPnpHandler(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// DefaultPnpHandler
	IoSkipCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}							// DefaultPnpHandler

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleCancelRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleCancelRemove
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_CANCEL_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == PENDINGREMOVE)
		{						// we succeeded earlier query

		// Lower-level drivers are presumably in the pending-remove state as
		// well, so we need to tell them that the remove has been cancelled
		// before we start sending IRPs down to them.

		NTSTATUS status = ForwardAndWait(fdo, Irp); // wait for lower layers
		if (NT_SUCCESS(status))
			{					// completed successfully
			KdPrint((DRIVERNAME " - To %s from PENDINGREMOVE\n", statenames[pdx->prevstate]));
			if ((pdx->state = pdx->prevstate) == WORKING)
				{				// back to working state
				}				// back to working state
			}					// completed successfully
		else
			KdPrint((DRIVERNAME " - Status %8.8lX returned by PDO for IRP_MN_CANCEL_REMOVE_DEVICE", status));

		return CompleteRequest(Irp, status);
		}						// we succeeded earlier query
	
	return DefaultPnpHandler(fdo, Irp); // unexpected cancel
	}							// HandleCancelRemove

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleCancelStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleCancelStop
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_CANCEL_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == PENDINGSTOP)
		{						// we succeeded earlier query

		// Lower level drivers are presumably in the pending-stop state as
		// well, so we need to tell them that the stop has been cancelled
		// before we start sending IRPs down to them.

		NTSTATUS status = ForwardAndWait(fdo, Irp); // wait for lower layers
		if (NT_SUCCESS(status))
			{					// completed successfully
			KdPrint((DRIVERNAME " - To WORKING from PENDINGSTOP\n"));
			pdx->state = WORKING;
			}					// completed successfully
		else
			KdPrint((DRIVERNAME " - Status %8.8lX returned by PDO for IRP_MN_CANCEL_STOP_DEVICE", status));
		
		return CompleteRequest(Irp, status);
		}						// we succeeded earlier query

	return DefaultPnpHandler(fdo, Irp); // unexpected cancel
	}							// HandleCancelStop

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryCapabilities(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryCapabilities
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_CAPABILITIES);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_CAPABILITIES pdc = stack->Parameters.DeviceCapabilities.Capabilities;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Check to be sure we know how to handle this version of the capabilities structure

	if (pdc->Version < 1)
		return DefaultPnpHandler(fdo, Irp);

	NTSTATUS status = ForwardAndWait(fdo, Irp);
	if (NT_SUCCESS(status))
		{						// IRP succeeded
		stack = IoGetCurrentIrpStackLocation(Irp);
		pdc = stack->Parameters.DeviceCapabilities.Capabilities;

		// TODO Modify any capabilities that must be set on the way back up

		pdc->SurpriseRemovalOK = TRUE;
		pdx->devcaps = *pdc;	// save capabilities for whoever needs to see them
		}						// IRP succeeded

	return CompleteRequest(Irp, status);
	}							// HandleQueryCapabilities

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryRemove(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryRemove
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	if (pdx->state == WORKING)
		{						// currently working

	#ifdef _X86_

		// Win98 doesn't check for open handles before allowing a remove to proceed,
		// and it may deadlock in IoReleaseRemoveLockAndWait if handles are still
		// open.

		if (win98 && pdx->DeviceObject->ReferenceCount)
			{
			KdPrint((DRIVERNAME " - Failing removal query due to open handles\n"));
			return CompleteRequest(Irp, STATUS_DEVICE_BUSY);
			}

	#endif
		}						// currently working

	// Save current state for restoration if the query gets cancelled.
	// (We can now be stopped or working)

	pdx->prevstate = pdx->state;
	pdx->state = PENDINGREMOVE;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleQueryRemove

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleQueryStop(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleQueryStop
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_QUERY_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	
	// Boot devices may get this query before they even start, so check to see
	// if we're in the WORKING state before doing anything.

	if (pdx->state != WORKING)
		return DefaultPnpHandler(fdo, Irp);

	KdPrint((DRIVERNAME " - To PENDINGSTOP from %s\n", statenames[pdx->state]));
	pdx->state = PENDINGSTOP;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleQueryStop

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleRemoveDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleRemoveDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_REMOVE_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Cancel any queued IRPs and start rejecting new ones

	AbortPendingIoctls(pdx, STATUS_DELETE_PENDING);

	// Disable all device interfaces. This triggers PnP notifications that will 
	// allow apps to close their handles.

	DeregisterAllInterfaces(pdx);

	// Release our I/O resources

	StopDevice(fdo, pdx->state == WORKING);

	KdPrint((DRIVERNAME " - To REMOVED from %s\n", statenames[pdx->state]));
	pdx->state = REMOVED;

	// Let lower-level drivers handle this request. Ignore whatever
	// result eventuates.

	NTSTATUS status = DefaultPnpHandler(fdo, Irp);

	// Wait for all claims against this device to vanish before removing
	// the device object.

	IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);

	// Remove the device object

	RemoveDevice(fdo);

	return status;				// lower-level completed IoStatus already
	}							// HandleRemoveDevice

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleStartDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleStartDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_START_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	NTSTATUS status = ForwardAndWait(fdo, Irp);
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status);

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	status = StartDevice(fdo);

	// While we were in the stopped state, we were stalling incoming requests.
	// Now we can release any pending IRPs and start processing new ones

	if (NT_SUCCESS(status))
		{						// started okay

		// Enable all registered device interfaces.

		EnableAllInterfaces(pdx, TRUE);

		KdPrint((DRIVERNAME " - To WORKING from %s\n", statenames[pdx->state]));
		pdx->state = WORKING;
		}						// started okay

	return CompleteRequest(Irp, status);
	}							// HandleStartDevice

///////////////////////////////////////////////////////////////////////////////	

NTSTATUS HandleStopDevice(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleStopDevice
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_STOP_DEVICE);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// We're supposed to always get a query before we're stopped, so
	// we should already be in the PENDINGSTOP state. There's a Win98 bug that
	// can sometimes cause us to get a STOP instead of a REMOVE, in which case
	// we should start rejecting IRPs

	if (pdx->state != PENDINGSTOP)
		{						// no previous query
		KdPrint((DRIVERNAME " - STOP with no previous QUERY_STOP!\n"));
		AbortPendingIoctls(pdx, STATUS_DELETE_PENDING);
		}						// no previous query
	StopDevice(fdo, pdx->state == WORKING);
	KdPrint((DRIVERNAME " - To STOPPED from %s\n", statenames[pdx->state]));
	pdx->state = STOPPED;
	return DefaultPnpHandler(fdo, Irp);
	}							// HandleStopDevice

///////////////////////////////////////////////////////////////////////////////

NTSTATUS HandleSurpriseRemoval(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
	{							// HandleSurpriseRemoval
	ASSERT(IoGetCurrentIrpStackLocation(Irp)->MinorFunction == IRP_MN_SURPRISE_REMOVAL);
	Irp->IoStatus.Status = STATUS_SUCCESS;	// flag that we handled this IRP
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	// Cancel any queued IRPs and start rejecting new ones
	AbortPendingIoctls(pdx, STATUS_DELETE_PENDING);
	EnableAllInterfaces(pdx, FALSE);
	KdPrint((DRIVERNAME " - To SURPRISEREMOVED from %s\n", statenames[pdx->state]));

	BOOLEAN oktouch = pdx->state == WORKING;
	pdx->state = SURPRISEREMOVED;
	StopDevice(fdo, oktouch);

	return DefaultPnpHandler(fdo, Irp);
	}							// HandleSurpriseRemoval

///////////////////////////////////////////////////////////////////////////////	

