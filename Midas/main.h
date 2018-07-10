#pragma once
#include "ntos.h"

EXTERN_C NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path);

EXTERN_C NTSYSCALLAPI NTSTATUS ZwOpenDirectoryObject(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES);
EXTERN_C NTSYSCALLAPI VOID ExAcquirePushLockExclusiveEx(PEX_PUSH_LOCK, ULONG Flags);
EXTERN_C NTSYSCALLAPI VOID ExReleasePushLockExclusiveEx(PEX_PUSH_LOCK, ULONG Flags);
EXTERN_C NTSYSCALLAPI PLIST_ENTRY PsLoadedModuleList;
EXTERN_C NTSYSCALLAPI PVOID RtlImageDirectoryEntryToData(_In_  PVOID   Base, _In_  BOOLEAN MappedAsImage, _In_  USHORT  DirectoryEntry, _Out_ PULONG  Size);
EXTERN_C NTSYSCALLAPI PVOID ObQueryNameInfo(_In_ PVOID Object);

// empty icall dispatch handler to disable cfg
extern "C" void _ignore_icall(void);