#include "main.h"
#include "utils.h"
#include <intrin.h>


extern "C" BOOLEAN b_data_compare(const unsigned char* pData, const unsigned char* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return 0;
	return (*szMask) == 0;
}
extern "C" UINT64 find_pattern(UINT64 dwAddress, UINT64 dwLen, unsigned char *bMask, char * szMask) {
	for (UINT64 i = 0; i < dwLen; i++) {
		if (b_data_compare((unsigned char*)(dwAddress + i), bMask, szMask)) {
			return (UINT64)(dwAddress + i);
		}
	}
	return 0;
}

extern "C" BOOLEAN clean_unloaded_drivers()
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
	UINT64 mm_unloaded_drivers_ptr = find_pattern(ntoskrnl_base, ntoskrnl_size, (unsigned char*)"\x4C\x8B\x00\x00\x00\x00\x00\x4C\x8B\xC9\x4D\x85\x00\x74", "xx?????xxxxx?x");

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

extern "C" NTSTATUS copy_memory(IN PCOPY_MEMORY p_copy)
{
	
	//RtlCopyMemory(pTarget, pSource, pCopy->size); //alternate method of copying memory. Why not to use: https://www.unknowncheats.me/forum/1543554-post3.html
	//probing cant be used because it throws a SEH exception which is not compatible when mapping driver manually (without lots of work)

	PEPROCESS p_process = NULL, p_source_proc, p_target_proc;
	PVOID p_source, p_target;

	NTSTATUS status = PsLookupProcessByProcessId((HANDLE)p_copy->pid, &p_process);
	if (NT_ERROR(status)) {
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
	
	return status;
}

extern "C" NTSTATUS get_base_address(IN OUT PBASE_ADDRESS data)
{
	
	PEPROCESS p_process = NULL;
	const NTSTATUS status = PsLookupProcessByProcessId((HANDLE)data->PID, &p_process);

	if (NT_ERROR(status))
		return status;

	data->BaseAddress = (ULONGLONG)PsGetProcessSectionBaseAddress(p_process);

	if (data->BaseAddress == 0)
		return STATUS_UNSUCCESSFUL;

	
	return status;
}

extern "C" NTSTATUS CopyMajorFunctions(_In_reads_bytes_(count * sizeof(PDRIVER_DISPATCH)) PDRIVER_DISPATCH* src, _Out_writes_bytes_all_(count * sizeof(PDRIVER_DISPATCH)) PDRIVER_DISPATCH* dst, const SIZE_T size)
{
	constexpr auto major_functions = IRP_MJ_MAXIMUM_FUNCTION + 1;

	if (size != major_functions)
	{
		return STATUS_INFO_LENGTH_MISMATCH;
	}

	for (unsigned i = 0; i < major_functions; ++i)
	{
		dst[i] = src[i];
	}

	return STATUS_SUCCESS;
}

extern "C" ULONGLONG SetCfgDispatch(const PDRIVER_OBJECT driver, const ULONGLONG new_dispatch)
{
	ULONG size = 0;
	const auto directory = PIMAGE_LOAD_CONFIG_DIRECTORY(RtlImageDirectoryEntryToData(driver->DriverStart, TRUE, IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size));

	if (directory != nullptr) {
		if (directory->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED) {
			{
				const auto old_dispatch = directory->GuardCFDispatchFunctionPointer;

				auto cr0 = __readcr0();

				const auto old_cr0 = cr0;
				// disable write protection
				cr0 &= ~(1UL << 16);
				__writecr0(cr0);

				directory->GuardCFDispatchFunctionPointer = new_dispatch;

				__writecr0(old_cr0);
				return old_dispatch;
			}
		}
	}

	return 0;
}