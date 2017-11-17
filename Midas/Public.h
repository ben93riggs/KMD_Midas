/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_Midas,
    0xf5fe880b,0x2053,0x4382,0x9b,0x8e,0x59,0xab,0x84,0x78,0x33,0x09);
// {f5fe880b-2053-4382-9b8e-59ab84783309}
