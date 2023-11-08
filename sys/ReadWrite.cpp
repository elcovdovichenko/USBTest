// Read/Write request processors for usbprobe driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#include "stddcls.h"
#include "driver.h"

#ifdef DBG
	#define MSGUSBSTRING(d,s,i) { \
		UNICODE_STRING sd; \
		if (i && NT_SUCCESS(GetStringDescriptor(d,i,&sd))) { \
			DbgPrint(s, sd.Buffer); \
			RtlFreeUnicodeString(&sd); \
		}}
#else
	#define MSGUSBSTRING(d,s,i)
#endif

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID AbortPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe)
	{							// AbortPipe
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	urb.UrbHeader.Length = (USHORT) sizeof(_URB_PIPE_REQUEST);
	urb.UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
	urb.UrbPipeRequest.PipeHandle = hpipe;

	NTSTATUS status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X in AbortPipe\n", status));
	}							// AbortPipe

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchCreate
	PAGED_CODE();
	KdPrint((DRIVERNAME " - IRP_MJ_CREATE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	NTSTATUS status = STATUS_SUCCESS;

	if (NT_SUCCESS(status))
		{						// okay to open
		if (InterlockedIncrement(&pdx->handles) == 1)
			{					// first open handle
			}					// okay to open
		}					// first open handle
	return CompleteRequest(Irp, status, 0);
	}							// DispatchCreate

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp)
	{							// DispatchClose
	PAGED_CODE();
	KdPrint((DRIVERNAME " - IRP_MJ_CLOSE\n"));
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	if (InterlockedDecrement(&pdx->handles) == 0)
		{						// no more open handles
		}						// no more open handles

	return CompleteRequest(Irp, STATUS_SUCCESS, 0);
	}							// DispatchClose

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS GetStringDescriptor(PDEVICE_OBJECT fdo, UCHAR istring, PUNICODE_STRING s)
	{							// GetStringDescriptor
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	UCHAR data[255];			// maximum-length buffer

	// If this is the first time here, read string descriptor zero and arbitrarily select
	// the first language identifer as the one to use in subsequent get-descriptor calls.

	if (!pdx->langid)
		{						// determine default language id
		UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_STRING_DESCRIPTOR_TYPE,
			0, 0, data, NULL, sizeof(data), NULL);
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			return status;
		pdx->langid = *(LANGID*)(data + 2);
        
		//KdPrint((DRIVERNAME " - langid = 0x%x\n", pdx->langid));

		}						// determine default language id

	// Fetch the designated string descriptor.

	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_STRING_DESCRIPTOR_TYPE,
		istring, pdx->langid, data, NULL, sizeof(data), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		return status;
    
	//KdPrint((DRIVERNAME " - length = 0x%x, type = 0x%x\n", data[0], data[1]));
	
	/*
	ULONG len = data[0];
	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_STRING_DESCRIPTOR_TYPE,
		istring, pdx->langid, data, NULL, len, NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		return status;
	*/

	ULONG nchars = (data[0] - 2) / 2;
	PWSTR p = (PWSTR) ExAllocatePool(PagedPool, data[0]);
	if (!p)
		return STATUS_INSUFFICIENT_RESOURCES;

	memcpy(p, data + 2, nchars*2);
	p[nchars] = 0;

	s->Length = (USHORT) (2 * nchars);
	s->MaximumLength = (USHORT) ((2 * nchars) + 2);
	s->Buffer = p;

	return STATUS_SUCCESS;
	}							// GetStringDescriptor

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID ResetDevice(PDEVICE_OBJECT fdo)
	{							// ResetDevice
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);
	IO_STATUS_BLOCK iostatus;

	PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_RESET_PORT,
		pdx->LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &iostatus);
	if (!Irp)
		return;

	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	if (status == STATUS_PENDING)
		{
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = iostatus.Status;
		}

	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X trying to reset device\n", status));
	}							// ResetDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

NTSTATUS ResetPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe)
	{							// ResetPipe
	PAGED_CODE();
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	URB urb;

	urb.UrbHeader.Length = (USHORT) sizeof(_URB_PIPE_REQUEST);
	urb.UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
	urb.UrbPipeRequest.PipeHandle = hpipe;

	NTSTATUS status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		KdPrint((DRIVERNAME " - Error %X trying to reset a pipe\n", status));
	return status;
	}							// ResetPipe

///////////////////////////////////////////////////////////////////////////////

NTSTATUS SendAwaitUrb(PDEVICE_OBJECT fdo, PURB urb)
	{							// SendAwaitUrb
	PAGED_CODE();
	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	//KdPrint((DRIVERNAME " - SendAwaitUrb begin\n"));

	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	//KdPrint((DRIVERNAME " - SendAwaitUrb 1\n"));
	
	IO_STATUS_BLOCK iostatus;
	PIRP Irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
		pdx->LowerDeviceObject, NULL, 0, NULL, 0, TRUE, &event, &iostatus);
	
	//KdPrint((DRIVERNAME " - SendAwaitUrb 2\n"));
	
	if (!Irp)
		{
		KdPrint((DRIVERNAME " - Unable to allocate IRP for sending URB\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	PIO_STACK_LOCATION stack = IoGetNextIrpStackLocation(Irp);
	stack->Parameters.Others.Argument1 = (PVOID) urb;

	//KdPrint((DRIVERNAME " - SendAwaitUrb 3\n"));
	
	NTSTATUS status = IoCallDriver(pdx->LowerDeviceObject, Irp);
	
	//KdPrint((DRIVERNAME " - SendAwaitUrb 4\n"));
		
	if (status == STATUS_PENDING)
		{
	
		//KdPrint((DRIVERNAME " - SendAwaitUrb 5\n"));
		
		KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
		status = iostatus.Status;
		}

	//KdPrint((DRIVERNAME " - SendAwaitUrb complete\n"));
		
	return status;
	}							// SendAwaitUrb

///////////////////////////////////////////////////////////////////////////////

NTSTATUS StartDevice(PDEVICE_OBJECT fdo)
	{							// StartDevice
	PAGED_CODE();
	NTSTATUS status;
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;

	URB urb;					// URB for use in this subroutine

	KdPrint((DRIVERNAME " - Entry StartDevice 19\n"));
	
	// Read our device descriptor. The only thing this skeleton does with it is print
	// debugging messages with the string descriptors.

	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_DEVICE_DESCRIPTOR_TYPE,
		0, 0, &pdx->dd, NULL, sizeof(pdx->dd), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Error %X trying to read device descriptor\n", status));
		return status;
		}

	MSGUSBSTRING(fdo, DRIVERNAME " - Configuring device from %ws\n", pdx->dd.iManufacturer);
	MSGUSBSTRING(fdo, DRIVERNAME " - Product is %ws\n", pdx->dd.iProduct);
	MSGUSBSTRING(fdo, DRIVERNAME " - Serial number is %ws\n", pdx->dd.iSerialNumber);

	// Read the descriptor of the first configuration. This requires two steps. The first step
	// reads the fixed-size configuration descriptor alone. The second step reads the
	// configuration descriptor plus all imbedded interface and endpoint descriptors.

	USB_CONFIGURATION_DESCRIPTOR tcd;
	UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE,
		0, 0, &tcd, NULL, sizeof(tcd), NULL);
	status = SendAwaitUrb(fdo, &urb);
	if (!NT_SUCCESS(status))
		{
		KdPrint((DRIVERNAME " - Error %X trying to read configuration descriptor 1\n", status));
		return status;
		}

	ULONG size = tcd.wTotalLength;

    KdPrint((DRIVERNAME " - wTotalLength = %d\n", tcd.wTotalLength));

	PUSB_CONFIGURATION_DESCRIPTOR pcd = (PUSB_CONFIGURATION_DESCRIPTOR) ExAllocatePool(NonPagedPool, size);
	if (!pcd)
		{
		KdPrint((DRIVERNAME " - Unable to allocate %X bytes for configuration descriptor\n", size));
		return STATUS_INSUFFICIENT_RESOURCES;
		}

	__try
		{
		UsbBuildGetDescriptorRequest(&urb, sizeof(_URB_CONTROL_DESCRIPTOR_REQUEST), USB_CONFIGURATION_DESCRIPTOR_TYPE,
			0, 0, pcd, NULL, size, NULL);
		status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			{
			KdPrint((DRIVERNAME " - Error %X trying to read configuration descriptor 1\n", status));
			return status;
			}
    
		KdPrint((DRIVERNAME " - bNumInterfaces = %d\n", pcd->bNumInterfaces));
		MSGUSBSTRING(fdo, DRIVERNAME " - Selecting configuration named %ws\n", pcd->iConfiguration);

		// Locate the descriptor for the one and only interface we expect to find

		PUSB_INTERFACE_DESCRIPTOR pid = USBD_ParseConfigurationDescriptorEx(pcd, pcd,
			-1, -1, -1, -1, -1);
		ASSERT(pid);
                                   
		MSGUSBSTRING(fdo, DRIVERNAME " - Selecting interface named %ws\n", pid->iInterface);

		// Create a URB to use in selecting a configuration.

		USBD_INTERFACE_LIST_ENTRY interfaces[2] = {
			{pid, NULL},
			{NULL, NULL},		// fence to terminate the array
			};

		PURB selurb = USBD_CreateConfigurationRequestEx(pcd, interfaces);
		if (!selurb)
			{
			KdPrint((DRIVERNAME " - Unable to create configuration request\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
			}

		__try
			{

			// Verify that the interface describes exactly the endpoints we expect

			if (pid->bNumEndpoints != 2)
				{
				KdPrint((DRIVERNAME " - %d is the wrong number of endpoints\n", pid->bNumEndpoints));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}

			PUSB_ENDPOINT_DESCRIPTOR ped = (PUSB_ENDPOINT_DESCRIPTOR) pid;
			ped = (PUSB_ENDPOINT_DESCRIPTOR) USBD_ParseDescriptors(pcd, tcd.wTotalLength, ped, USB_ENDPOINT_DESCRIPTOR_TYPE);
			
			KdPrint((DRIVERNAME " - Endpoint 1: bEndpointAddress=0x%x, bmAttributes=0x%x, wMaxPacketSize=0x%x\n",
				     ped->bEndpointAddress,ped->bmAttributes,ped->wMaxPacketSize));
			
			if (!ped || ped->bEndpointAddress != 0x1 || ped->bmAttributes != USB_ENDPOINT_TYPE_BULK || ped->wMaxPacketSize != 16)
				{
				KdPrint((DRIVERNAME " - Endpoint has wrong attributes\n"));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}
			++ped;
			ped = (PUSB_ENDPOINT_DESCRIPTOR) USBD_ParseDescriptors(pcd, tcd.wTotalLength, ped, USB_ENDPOINT_DESCRIPTOR_TYPE);

			KdPrint((DRIVERNAME " - Endpoint 2: bEndpointAddress=0x%x, bmAttributes=0x%x, wMaxPacketSize=0x%x, bInterval==0x%x\n",
				     ped->bEndpointAddress, ped->bmAttributes, ped->wMaxPacketSize, ped->bInterval));
			
			if (!ped || ped->bEndpointAddress != 0x81 || ped->bmAttributes != USB_ENDPOINT_TYPE_INTERRUPT || ped->wMaxPacketSize != 32)
				{
				KdPrint((DRIVERNAME " - Endpoint has wrong attributes\n"));
				return STATUS_DEVICE_CONFIGURATION_ERROR;
				}
			++ped;

			
			PUSBD_INTERFACE_INFORMATION pii = interfaces[0].Interface;

			// Initialize the maximum transfer size for each of the endpoints
			// TODO remove these statements if you're happy with the default
			// value provided by USBD.

			pii->Pipes[0].MaximumTransferSize = 32;
			pii->Pipes[1].MaximumTransferSize = 32;
			
			// Submit the set-configuration request

			status = SendAwaitUrb(fdo, selurb);
			if (!NT_SUCCESS(status))
				{
				KdPrint((DRIVERNAME " - Error %X trying to select configuration\n", status));
				return status;
				}

			// Save the configuration and pipe handles

			pdx->hconfig = selurb->UrbSelectConfiguration.ConfigurationHandle;
			pdx->h1pipe = pii->Pipes[0].PipeHandle;
			pdx->h2pipe = pii->Pipes[1].PipeHandle;			

			KdPrint((DRIVERNAME " - h1pipe = 0x%x, h2pipe = 0x%x\n",
				    pdx->h1pipe, pdx->h2pipe));
			
			// TODO If you have an interrupt endpoint, now would be the time to
			// create an IRP and URB with which to poll it continuously

			// Transfer ownership of the configuration descriptor to the device extension
			
			pdx->pcd = pcd;
			pcd = NULL;
			}
		__finally
			{
			ExFreePool(selurb);
			}

		}
	__finally
		{
		if (pcd)
			ExFreePool(pcd);
		}

	return STATUS_SUCCESS;
	}							// StartDevice

///////////////////////////////////////////////////////////////////////////////

#pragma PAGEDCODE

VOID StopDevice(IN PDEVICE_OBJECT fdo, BOOLEAN oktouch /* = FALSE */)
	{							// StopDevice
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fdo->DeviceExtension;
	AbortPendingIoctls(pdx, STATUS_CANCELLED);

	// If it's okay to touch our hardware (i.e., we're processing an IRP_MN_STOP_DEVICE),
	// deconfigure the device.
	
	if (oktouch)
		{						// deconfigure device
		URB urb;
		UsbBuildSelectConfigurationRequest(&urb, sizeof(_URB_SELECT_CONFIGURATION), NULL);
		NTSTATUS status = SendAwaitUrb(fdo, &urb);
		if (!NT_SUCCESS(status))
			KdPrint((DRIVERNAME " - Error %X trying to deconfigure device\n", status));
		}						// deconfigure device

	if (pdx->pcd)
		ExFreePool(pdx->pcd);
	pdx->pcd = NULL;
	}							// StopDevice
