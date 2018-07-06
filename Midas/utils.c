#include "utils.h"
#include <intrin.h>

NTSTATUS copy_memory(IN PCOPY_MEMORY p_copy)
{
	//RtlCopyMemory(pTarget, pSource, pCopy->size); //alternate method of copying memory. Why not to use: https://www.unknowncheats.me/forum/1543554-post3.html
	//probing cant be used because it throws a SEH exception which is not compatible when mapping driver manually (without lots of work)

	PEPROCESS p_process = NULL, p_source_proc, p_target_proc;
	PVOID p_source, p_target;

	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)p_copy->pid, &p_process);
	if (NT_ERROR(status)) {
		Print("ERROR: PsLookupProcessByProcessId FAILED\n");
		return status;
	}

	if (p_copy->write == TRUE)
	{
		p_source_proc = IoGetCurrentProcess();
		p_target_proc = p_process;
		p_source = (PVOID)p_copy->localbuf;
		p_target = (PVOID)p_copy->target_ptr;
	}
	else
	{
		p_source_proc = p_process;
		p_target_proc = IoGetCurrentProcess();
		p_source = (PVOID)p_copy->target_ptr;
		p_target = (PVOID)p_copy->localbuf;
	}

	if (!p_copy->target_ptr || !p_copy->localbuf)
		return status;

	SIZE_T bytes = 0;
	status = MmCopyVirtualMemory(p_source_proc, p_source, p_target_proc, p_target, p_copy->size, KernelMode, &bytes);

	if (NT_ERROR(status))
		Print("\TERROR: MmCopyVirtualMemory Failed! - %X", status);

	return status;
}

NTSTATUS get_base_address(IN OUT PBASE_ADDRESS data)
{
	PEPROCESS p_process = NULL;

	const NTSTATUS status = PsLookupProcessByProcessId((HANDLE)data->PID, &p_process);

	if (NT_ERROR(status))
	{
		Print("PsLookupProcessByProcessId Error in GetBaseAddress(): %X\n", status);
		return status;
	}

	data->BaseAddress = (ULONGLONG)PsGetProcessSectionBaseAddress(p_process);
	if (data->BaseAddress == 0)
		return STATUS_UNSUCCESSFUL;

	return status;
}

BOOLEAN b_data_compare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;

	return (*szMask) == 0;
}

UINT64 find_pattern(UINT64 dwAddress, UINT64 dwLen, BYTE *bMask, char * szMask)
{
	for (UINT64 i = 0; i < dwLen; i++)
	{
		if (b_data_compare((BYTE*)(dwAddress + i), bMask, szMask))
		{
			return (UINT64)(dwAddress + i);
		}
	}

	return 0;
}

BOOLEAN clean_unloaded_drivers()
{
	ULONG bytes = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes); //find out how many bytes to allocate
	if (!bytes) 
		return FALSE;

	PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePool(NonPagedPool, bytes); //allocate X bytes to store info in

	status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes); //fill our allocated space with systemmoduleinformation

	if (!NT_SUCCESS(status))
		return FALSE;

	//ntoskrnl.exe is always the first module in the list
	UINT64 ntoskrnl_base = (UINT64)modules->Modules[0].ImageBase;
	UINT64 ntoskrnl_size = (UINT64)modules->Modules[0].ImageSize;

	if (modules)
		ExFreePool(modules);

	if (ntoskrnl_base <= 0)
		return FALSE;

	// NOTE: 4C 8B ? ? ? ? ? 4C 8B C9 4D 85 ? 74 + 3] + current signature address = MmUnloadedDrivers
	UINT64 mm_unloaded_drivers_ptr = find_pattern(ntoskrnl_base, ntoskrnl_size, (BYTE*)"\x4C\x8B\x00\x00\x00\x00\x00\x4C\x8B\xC9\x4D\x85\x00\x74", "xx?????xxxxx?x");

	if (!mm_unloaded_drivers_ptr)
		return FALSE;

	UINT64 mm_unloaded_drivers = (UINT64)((PUCHAR)mm_unloaded_drivers_ptr + *(PULONG)((PUCHAR)mm_unloaded_drivers_ptr + 3) + 7);
	UINT64 buffer_ptr = *(UINT64*)mm_unloaded_drivers;

	// NOTE: 0x7D0 is the size of the MmUnloadedDrivers array for win 7 and above
	PVOID new_buffer = ExAllocatePoolWithTag(NonPagedPoolNx, 0x7D0, 0x54446D4D);

	if (!new_buffer)
		return FALSE;

	memset(new_buffer, 0, 0x7D0);

	// NOTE: replace MmUnloadedDrivers
	*(UINT64*)mm_unloaded_drivers = (UINT64)new_buffer;

	// NOTE: clean the old buffer
	ExFreePoolWithTag((PVOID)buffer_ptr, 0x54446D4D); // 'MmDT'

	return TRUE;
}

//BOOLEAN b_data_compare(const BYTE* pData, const BYTE* bMask, const char* szMask)
//{
//	for (; *szMask; ++szMask, ++pData, ++bMask)
//		if (*szMask == 'x' && *pData != *bMask)
//			return 0;
//
//	return (*szMask) == 0;
//}
//
//UINT64 find_pattern(UINT64 dwAddress, UINT64 dwLen, BYTE *bMask, char * szMask)
//{
//	for (UINT64 i = 0; i < dwLen; i++)
//		if (b_data_compare((BYTE*)(dwAddress + i), bMask, szMask))
//			return (UINT64)(dwAddress + i);
//
//	return 0;
//}


////Gets the fullname(path+exe)
//BOOL _stdcall image_full_path(PEPROCESS eprocess, PCHAR fullname)
//{
//	BYTE buffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
//	HANDLE handle;
//	DWORD returnedLength;
//	ANSI_STRING DestinationString;
//
//	auto ret = FALSE;
//	returnedLength = 0;
//	if (NT_SUCCESS(ObOpenObjectByPointer(eprocess, OBJ_KERNEL_HANDLE, NULL, GENERIC_READ, NULL, KernelMode, &handle)))
//	{
//		if (NT_SUCCESS(ZwQueryInformationProcess(handle, ProcessImageFileName, buffer, sizeof(buffer), &returnedLength)))
//		{
//			RtlUnicodeStringToAnsiString(&DestinationString, (UNICODE_STRING*)buffer, TRUE);
//			strncpy(fullname, DestinationString.Buffer, DestinationString.Length);
//			ret = TRUE;
//			fullname[DestinationString.Length] = 0;
//			RtlFreeAnsiString(&DestinationString);
//		}
//		ZwClose(handle);
//	}
//	return ret;
//}
//
////Only the exe name
//BOOL _stdcall image_file_name(PEPROCESS eprocess, PCHAR filename)
//{
//	CHAR sImageFullPath[MAX_PATH];
//
//	sImageFullPath[0] = 0;
//	if (image_full_path(eprocess, sImageFullPath))
//	{
//		PCHAR pIFN = sImageFullPath, pIFP = sImageFullPath;
//		while (*pIFP)
//			if (*(pIFP++) == '\\')
//				pIFN = pIFP;
//		strcpy(filename, pIFN);
//		return TRUE;
//	}
//	return FALSE;
//}
//
//BOOL _stdcall is_current_process(PCHAR szExeName)
//{
//	PEPROCESS EProc;
//	CHAR szImageName[MAX_PATH];
//	szImageName[0] = 0;
//
//	HANDLE h_current_pid = PsGetCurrentProcessId();
//	if (!PsLookupProcessByProcessId(h_current_pid, &EProc) == STATUS_SUCCESS)
//		return FALSE;
//
//	image_file_name(EProc, szImageName);
//	ObDereferenceObject(EProc);
//
//	if (!szImageName[0])
//		return FALSE;
//		
//	if (strstr(szExeName, szImageName))
//		return TRUE;
//
//	return FALSE;
//}

//PVOID get_kernel_base(PULONG pImageSize)
//{
//	typedef struct _SYSTEM_MODULE_ENTRY
//	{
//		HANDLE Section;
//		PVOID MappedBase;
//		PVOID ImageBase;
//		ULONG ImageSize;
//		ULONG Flags;
//		USHORT LoadOrderIndex;
//		USHORT InitOrderIndex;
//		USHORT LoadCount;
//		USHORT OffsetToFileName;
//		UCHAR FullPathName[256];
//	} SYSTEM_MODULE_ENTRY, *PSYSTEM_MODULE_ENTRY;
//
//#pragma warning(disable:4200)
//	typedef struct _SYSTEM_MODULE_INFORMATION
//	{
//		ULONG Count;
//		SYSTEM_MODULE_ENTRY Module[0];
//	} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;
//
//	PVOID pModuleBase = NULL;
//	PSYSTEM_MODULE_INFORMATION pSystemInfoBuffer = NULL;
//
//	ULONG SystemInfoBufferSize = 0;
//
//	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, &SystemInfoBufferSize, 0, &SystemInfoBufferSize);
//
//	if (!SystemInfoBufferSize)
//		return NULL;
//
//	pSystemInfoBuffer = (PSYSTEM_MODULE_INFORMATION)ExAllocatePool(NonPagedPool, SystemInfoBufferSize * 2);
//
//	if (!pSystemInfoBuffer)
//		return NULL;
//
//	memset(pSystemInfoBuffer, 0, SystemInfoBufferSize * 2);
//
//	status = ZwQuerySystemInformation(SystemModuleInformation, pSystemInfoBuffer, SystemInfoBufferSize * 2, &SystemInfoBufferSize);
//
//	if (NT_SUCCESS(status))
//	{
//		pModuleBase = pSystemInfoBuffer->Module[0].ImageBase;
//		if (pImageSize)
//			*pImageSize = pSystemInfoBuffer->Module[0].ImageSize;
//	}
//
//	ExFreePool(pSystemInfoBuffer);
//
//	return pModuleBase;
//}

//https://en.wikipedia.org/wiki/Control_register
//void DISABLE_WP_FLAG() {
//	_disable(); //cli
//	unsigned long long cr0 = __readcr0(); //mov eax, cr0;
//	__writecr0(cr0 & 0xfffeffff); //and eax, 0xfffeffff; 
//								  //mov cr0, eax;
//	_enable(); //sti
//}
//void ENABLE_WP_FLAG() {
//	_disable(); //cli
//	unsigned long long cr0 = __readcr0(); //mov eax, cr0;
//	__writecr0(cr0 | 0x00010000); //or eax, 0x00010000; 
//								  //mov cr0, eax;
//	_enable(); //sti
//}

//void randomize_subserial(char* serial, size_t len)
//{
//	const auto seed = hash_subserial(serial, len) ^ g_startup_time;
//	auto engine = std::mt19937_64{ seed };
//	const auto distribution = std::uniform_int_distribution<unsigned>('A', 'Z');
//
//	KdPrint(("Randomizing subserial: seed: %016llX len: %d\n old: ", seed, len));
//	for (auto i = 0u; i < len; ++i)
//		KdPrint(("%02hhX ", uint8_t(serial[i])));
//	KdPrint(("\n new: "));
//
//	for (auto i = 0u; i < len; ++i)
//		if (is_good_char(serial[i]))
//			serial[i] = char(distribution(engine));
//
//	for (auto i = 0u; i < len; ++i)
//		KdPrint(("%02hhX ", uint8_t(serial[i])));
//	KdPrint(("\n"));
//}
//
//void spoof_serial(char* serial, bool is_smart)
//{
//	// must be 20 or less
//	size_t len;
//	char buf[21];
//	bool is_serial_hex;
//	if (is_smart)
//	{
//		is_serial_hex = false;
//		len = 20;
//		memcpy(buf, serial, 20);
//	}
//	else
//	{
//		is_serial_hex = true;
//		for (len = 0; serial[len]; ++len)
//			if (!is_hex(serial[len]))
//				is_serial_hex = false;
//
//		if (is_serial_hex)
//		{
//			len /= 2;
//			len = std::min<size_t>(len, 20);
//			for (auto i = 0u; i < len; ++i)
//				buf[i] = unhex_byte(serial[i * 2], serial[i * 2 + 1]);
//		}
//		else
//		{
//			memcpy(buf, serial, len);
//		}
//	}
//
//	buf[len] = 0;
//	char split[2][11];
//	memset(split, 0, sizeof(split));
//
//	for (auto i = 0u; i < len; ++i)
//		split[i % 2][i / 2] = buf[i];
//
//	randomize_subserial(split[0], (len + 1) / 2);
//	randomize_subserial(split[1], len / 2);
//
//	for (auto i = 0u; i < len; ++i)
//		buf[i] = split[i % 2][i / 2];
//	buf[len] = 0;
//
//	if (is_smart)
//	{
//		memcpy(serial, buf, 20);
//	}
//	else
//	{
//		if (is_serial_hex)
//		{
//			for (auto i = 0u; i < len; ++i)
//				std::tie(serial[i * 2], serial[i * 2 + 1]) = hex_byte(buf[i]);
//			serial[len * 2] = 0;
//		}
//		else
//		{
//			memcpy(serial, buf, len + 1);
//		}
//	}
//}