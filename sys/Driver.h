// Declarations for usbprobe driver
// Copyright (C) 1999, 2000 by Walter Oney
// All rights reserved

#ifndef DRIVER_H
#define DRIVER_H

#define DRIVERNAME "USBPROBE"				// for use in messages
#define LDRIVERNAME L"USBPROBE"				// for use in UNICODE string constants

///////////////////////////////////////////////////////////////////////////////
// Device extension structure

enum DEVSTATE {
	STOPPED,								// device stopped
	WORKING,								// started and working
	PENDINGSTOP,							// stop pending
	PENDINGREMOVE,							// remove pending
	SURPRISEREMOVED,						// removed by surprise
	REMOVED,								// removed
	};

typedef struct _DEVICE_EXTENSION {
	PDEVICE_OBJECT DeviceObject;			// device object this extension belongs to
	PDEVICE_OBJECT LowerDeviceObject;		// next lower driver in same stack
	PDEVICE_OBJECT Pdo;						// the PDO
	IO_REMOVE_LOCK RemoveLock;				// removal control locking structure
	UNICODE_STRING devname;
	DEVSTATE state;							// current state of device
	DEVSTATE prevstate;						// state prior to removal query
	DEVICE_POWER_STATE devpower;			// current device power state
	SYSTEM_POWER_STATE syspower;			// current system power state
	DEVICE_CAPABILITIES devcaps;			// copy of most recent device capabilities
	LIST_ENTRY PendingIoctlList;			// listing of pending async IOCTLs
	KSPIN_LOCK IoctlListLock;				// spin lock to guard PendingIoctlList
	NTSTATUS IoctlAbortStatus;				// status being used to abort pending IOCTLs
	LONG handles;							// # open handles
	USB_DEVICE_DESCRIPTOR dd;				// device descriptor
	USBD_CONFIGURATION_HANDLE hconfig;		// selected configuration handle
	PUSB_CONFIGURATION_DESCRIPTOR pcd;		// configuration descriptor
	LANGID langid;							// default language id for strings
	USBD_PIPE_HANDLE h1pipe;
	USBD_PIPE_HANDLE h2pipe;
    UNICODE_STRING InterfaceName;			// The name returned from IoRegisterDeviceInterface

	// TODO add additional per-device declarations
	KTIMER timer;

	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

///////////////////////////////////////////////////////////////////////////////
// Global functions

VOID RemoveDevice(IN PDEVICE_OBJECT fdo);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status);
NTSTATUS ForwardAndWait(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS SendDeviceSetPower(PDEVICE_EXTENSION fdo, DEVICE_POWER_STATE state, BOOLEAN wait = FALSE);
VOID SendAsyncNotification(PVOID context);
VOID EnableAllInterfaces(PDEVICE_EXTENSION pdx, BOOLEAN enable);
VOID DeregisterAllInterfaces(PDEVICE_EXTENSION pdx);
VOID AbortPendingIoctls(PDEVICE_EXTENSION pdx, NTSTATUS status);
VOID CleanupControlRequests(PDEVICE_EXTENSION pdx, NTSTATUS status, PFILE_OBJECT fop);
PIRP UncacheControlRequest(PDEVICE_EXTENSION pdx, PIRP* pIrp);
NTSTATUS SendAwaitUrb(PDEVICE_OBJECT fdo, PURB urb);
VOID AbortPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe);
NTSTATUS ResetPipe(PDEVICE_OBJECT fdo, USBD_PIPE_HANDLE hpipe);
VOID ResetDevice(PDEVICE_OBJECT fdo);
NTSTATUS StartDevice(PDEVICE_OBJECT fdo);
VOID StopDevice(PDEVICE_OBJECT fdo, BOOLEAN oktouch = FALSE);

// I/O request handlers

NTSTATUS DispatchCreate(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchControl(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPower(PDEVICE_OBJECT fdo, PIRP Irp);
NTSTATUS DispatchPnp(PDEVICE_OBJECT fdo, PIRP Irp);

extern BOOLEAN win98;
extern UNICODE_STRING servkey;

#endif // DRIVER_H
