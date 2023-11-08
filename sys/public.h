/*++

Copyright (c) 2001  ELCO  All Rights Reserved

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

Revision History:

    Vyacheslav Vdovichenko Apr 02 2001

Notes:

--*/

//
// Define an Interface Guid for function device class.
// This GUID is used to register (IoRegisterDeviceInterface)
// an instance of an interface so that user application
// can control the function device.
//

DEFINE_GUID(GUID_USBEXAMINATION_INTERFACE_CLASS, 
   0xf1cde16f, 0x68e2, 0x4be2, 0x90, 0xe1, 0xb1, 0xd9, 0x8, 0xef, 0x96, 0x6a);
// {F1CDE16F-68E2-4be2-90E1-B1D908EF966A}

//
// Define a Setup Class GUID for USBExamination Class. This is same
// as the ClassGuid in the INF files.
//

DEFINE_GUID (GUID_USBEXAMINATION_SETUP_CLASS,
   0x4D36E964, 0xE325, 0x11CE, 0xBF, 0xC1, 0x08, 0x00, 0x2B, 0xE1, 0x03, 0x18);
// {4D36E964-E325-11CE-BFC1-08002BE10318}
