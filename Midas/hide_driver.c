//#include "hide_driver.h"
//
//PVOID g_KernelBase = NULL;
//ULONG g_KernelSize = 0;
//
///// <summary>
///// Get ntoskrnl base address
///// </summary>
///// <param name="pSize">Size of module</param>
///// <returns>Found address, NULL if not found</returns>
//PVOID GetKernelBase(OUT PULONG pSize)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//	ULONG bytes = 0;
//	PRTL_PROCESS_MODULES pMods = NULL;
//	PVOID checkPtr = NULL;
//	UNICODE_STRING routineName;
//
//	// Already found
//	if (g_KernelBase != NULL)
//	{
//		if (pSize)
//			*pSize = g_KernelSize;
//		return g_KernelBase;
//	}
//
//	RtlInitUnicodeString(&routineName, L"NtOpenFile");
//	checkPtr = MmGetSystemRoutineAddress(&routineName);
//	if (checkPtr == NULL)
//		return NULL;
//
//	// Protect from UserMode AV
//	status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);
//	if (bytes == 0)
//	{
//		Print("BlackBone: %s: Invalid SystemModuleInformation size\n", __FUNCTION__);
//		return NULL;
//	}
//
//	pMods = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, BW_POOL_TAG);
//	RtlZeroMemory(pMods, bytes);
//
//	status = ZwQuerySystemInformation(SystemModuleInformation, pMods, bytes, &bytes);
//
//	if (NT_SUCCESS(status))
//	{
//		PRTL_PROCESS_MODULE_INFORMATION pMod = pMods->Modules;
//
//		for (ULONG i = 0; i < pMods->NumberOfModules; i++)
//		{
//			// System routine is inside module
//			if (checkPtr >= pMod[i].ImageBase &&
//				checkPtr < (PVOID)((PUCHAR)pMod[i].ImageBase + pMod[i].ImageSize))
//			{
//				g_KernelBase = pMod[i].ImageBase;
//				g_KernelSize = pMod[i].ImageSize;
//				if (pSize)
//					*pSize = g_KernelSize;
//				break;
//			}
//		}
//	}
//
//	if (pMods)
//		ExFreePoolWithTag(pMods, BW_POOL_TAG);
//
//	return g_KernelBase;
//}
//
//PLIST_ENTRY PsLoadedModuleList;
//NTSTATUS set_ps_loaded_module_list(IN PKLDR_DATA_TABLE_ENTRY pThisModule)
//{
//	PVOID kernel_base = GetKernelBase(NULL);
//	Print("GetKernelBase(NULL): %p", kernel_base);
//
//	if (kernel_base == NULL)
//	{
//		Print("BlackBone: %s: Failed to retrieve Kernel base address. Aborting\n", __FUNCTION__);
//		return STATUS_NOT_FOUND;
//	}
//
//	// Get PsLoadedModuleList address
//	for (PLIST_ENTRY pListEntry = pThisModule->InLoadOrderLinks.Flink; pListEntry != &pThisModule->InLoadOrderLinks; pListEntry = pListEntry->Flink)
//	{
//		// Search for Ntoskrnl entry
//		PKLDR_DATA_TABLE_ENTRY pEntry = CONTAINING_RECORD(pListEntry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
//		if (kernel_base == pEntry->DllBase)
//		{
//			// Ntoskrnl is always first entry in the list
//			// Check if found pointer belongs to Ntoskrnl module
//			if ((PVOID)pListEntry->Blink >= pEntry->DllBase && (PUCHAR)pListEntry->Blink < (PUCHAR)pEntry->DllBase + pEntry->SizeOfImage)
//			{
//				PsLoadedModuleList = pListEntry->Blink;
//				Print("FOUND PsLoadedModuleList");
//				return STATUS_SUCCESS;
//			}
//		}
//	}
//
//	Print("BlackBone: %s: Failed to retrieve PsLoadedModuleList address. Aborting\n", __FUNCTION__);
//	return STATUS_NOT_FOUND;
//}
//
//NTSTATUS hide_capcom()
//{
//	const NTSTATUS success = set_ps_loaded_module_list((PKLDR_DATA_TABLE_ENTRY)g_driver_object->DriverSection);
//
//	if (NT_ERROR(success))
//	{
//		Print("set_ps_loaded_module_list failed with error code: %X\n", success);
//		return success;
//	}
//
//	Print("PsLoadedModuleList:\t%p", PsLoadedModuleList);
//
//	UNICODE_STRING capcom_name = { 0 };
//	RtlInitUnicodeString(&capcom_name, L"Capcom.sys");
//	const auto name = &capcom_name;
//
//	// No images
//	if (IsListEmpty(PsLoadedModuleList))
//	{
//		Print("PsLoadedModuleList empty");
//		return STATUS_ERROR_PROCESS_NOT_IN_JOB;
//	}
//
//	for (PLIST_ENTRY list_entry = PsLoadedModuleList->Flink; list_entry != PsLoadedModuleList; list_entry = list_entry->Flink)
//	{
//		PKLDR_DATA_TABLE_ENTRY entry = CONTAINING_RECORD(list_entry, KLDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
//		Print("%wZ\n", &entry->BaseDllName);
//		// Check by name or by address
//		if (RtlCompareUnicodeString(&entry->BaseDllName, name, TRUE) == 0)
//		{
//			Print("Nulling entry for %wZ\n", name);
//			entry->FullDllName.Buffer[0] = 0;
//			entry->FullDllName.Length = 0;
//			entry->BaseDllName.Buffer[0] = 0;
//			entry->BaseDllName.Length = 0;
//			return STATUS_SUCCESS;
//		}
//	}
//
//	Print("hide_capcom() Could not find process!");
//	return STATUS_ERROR_PROCESS_NOT_IN_JOB;
//}
//
//typedef NTSTATUS(__fastcall *mi_process_loader_entry)(PVOID p_driver_section, int b_load);
//mi_process_loader_entry g_pfn_mi_process_loader_entry = NULL;
//
//NTSYSAPI NTSTATUS NTAPI ObReferenceObjectByName(__in PUNICODE_STRING ObjectName, __in ULONG Attributes, __in_opt PACCESS_STATE AccessState, __in_opt ACCESS_MASK DesiredAccess, __in POBJECT_TYPE ObjectType, __in KPROCESSOR_MODE AccessMode, __inout_opt PVOID ParseContext, __out PVOID* Object);
//
//POBJECT_TYPE *io_driver_object_type;
//
//PVOID get_call_point(PVOID pCallPoint)
//{
//	ULONG dwOffset = 0;
//	ULONG_PTR returnAddress = 0;
//	LARGE_INTEGER returnAddressTemp = { 0 };
//	PUCHAR pFunAddress = NULL;
//
//	if (pCallPoint == NULL || !MmIsAddressValid(pCallPoint))
//		return NULL;
//
//	pFunAddress = pCallPoint;
//	// º¯ÊýÆ«ÒÆ  
//	RtlCopyMemory(&dwOffset, (PVOID)(pFunAddress + 1), sizeof(ULONG));
//
//	// JMPÏòÉÏÌø×ª  
//	if ((dwOffset & 0x10000000) == 0x10000000)
//	{
//		dwOffset = dwOffset + 5 + pFunAddress;
//		returnAddressTemp.QuadPart = (ULONG_PTR)pFunAddress & 0xFFFFFFFF00000000;
//		returnAddressTemp.LowPart = dwOffset;
//		returnAddress = returnAddressTemp.QuadPart;
//		return (PVOID)returnAddress;
//	}
//
//	returnAddress = (ULONG_PTR)dwOffset + 5 + pFunAddress;
//	return (PVOID)returnAddress;
//
//}
//
//NTSTATUS get_driver_object(PDRIVER_OBJECT *lpObj, WCHAR* DriverDirName)
//{
//	NTSTATUS status = STATUS_SUCCESS;
//	PDRIVER_OBJECT pBeepObj = NULL;
//	UNICODE_STRING DevName = { 0 };
//
//	if (!MmIsAddressValid(lpObj))
//		return STATUS_INVALID_ADDRESS;
//
//	RtlInitUnicodeString(&DevName, DriverDirName);
//
//	status = ObReferenceObjectByName(&DevName, OBJ_CASE_INSENSITIVE, NULL, 0, *io_driver_object_type, KernelMode, NULL, &pBeepObj);
//
//	if (NT_SUCCESS(status))
//		*lpObj = pBeepObj;
//	else
//	{
//		Print("Get Obj faild...error:0x%x\n", status);
//	}
//
//	return status;
//}
//
//void support_seh(const PDRIVER_OBJECT p_driver_object)
//{
//	PDRIVER_OBJECT p_temp_drv_obj = NULL;
//
//	const PLDR_DATA_TABLE_ENTRY ldr = p_driver_object->DriverSection;
//
//	if (NT_SUCCESS(get_driver_object(&p_temp_drv_obj, L"\\Driver\\midas")))
//	{
//		ldr->DllBase = p_temp_drv_obj->DriverStart;
//	}
//}
//
//PVOID get_undocument_function_address(IN PUNICODE_STRING pFunName, IN PUCHAR pStartAddress, IN UCHAR* pFeatureCode, IN ULONG FeatureCodeNum, ULONG SerSize, UCHAR SegCode, ULONG AddNum, BOOLEAN ByName)
//{
//	ULONG dwIndex = 0;
//	PUCHAR pFunAddress = NULL;
//	ULONG dwCodeNum = 0;
//
//	if (pFeatureCode == NULL)
//		return NULL;
//
//	if (FeatureCodeNum >= 15)
//		return NULL;
//
//	if (SerSize > 0x1024)
//		return NULL;
//
//	if (ByName)
//	{
//		if (pFunName == NULL || !MmIsAddressValid(pFunName->Buffer))
//			return NULL;
//
//		pFunAddress = (PUCHAR)MmGetSystemRoutineAddress(pFunName);
//		if (pFunAddress == NULL)
//			return NULL;
//	}
//	else
//	{
//		if (pStartAddress == NULL || !MmIsAddressValid(pStartAddress))
//			return NULL;
//
//		pFunAddress = pStartAddress;
//	}
//
//	for (dwIndex = 0; dwIndex < SerSize; dwIndex++)
//	{
//		__try
//		{
//			if (pFunAddress[dwIndex] == pFeatureCode[dwCodeNum] || pFeatureCode[dwCodeNum] == SegCode)
//			{
//				dwCodeNum++;
//
//				if (dwCodeNum == FeatureCodeNum)
//					return pFunAddress + dwIndex - dwCodeNum + 1 + (int)AddNum;
//
//				continue;
//			}
//
//			dwCodeNum = 0;
//		}
//		__except (EXCEPTION_EXECUTE_HANDLER)
//		{
//			return 0;
//		}
//	}
//
//	return 0;
//}
//
//NTSTATUS hide_driver_win10(PDRIVER_OBJECT pTargetDriverObject)
//{
//	UNICODE_STRING usRoutie = { 0 };
//	PUCHAR pAddress = NULL;
//	PUCHAR pMiUnloadSystemImage = NULL;
//
//	UCHAR code[3] =
//		"\xD8\xE8";
//
//	UCHAR code2[10] =
//		"\x48\x8B\xD8\xE8\x60\x60\x60\x60\x8B";
//
//	UCHAR code3[3] =
//		"\xA8\x04";
//
//	/*
//	PAGE:000000014052ABE4 48 8B D8                                      mov     rbx, rax
//	PAGE:000000014052ABE7 E8 48 17 F7 FF                                call    MiUnloadSystemImage
//	*/
//	DbgBreakPoint();
//	if (pTargetDriverObject == NULL)
//		return STATUS_INVALID_PARAMETER;
//
//	RtlInitUnicodeString(&usRoutie, L"MmUnloadSystemImage");
//
//	pAddress = get_undocument_function_address(&usRoutie, NULL, code, 2, 0x30, 0x90, 1, TRUE);
//
//	if (pAddress == NULL)
//	{
//		Print("MiUnloadSystemImage 1 faild!\n");
//		return STATUS_UNSUCCESSFUL;
//	}
//
//	pAddress = get_call_point(pAddress);
//
//	if (pAddress == NULL)
//	{
//		Print("MiUnloadSystemImage 2 faild!\n");
//		return STATUS_UNSUCCESSFUL;
//	}
//	pMiUnloadSystemImage = pAddress;
//	/*
//	PAGE:000000014049C5CF 48 8B CB                                      mov     rcx, rbx
//	PAGE:000000014049C5D2 E8 31 29 C2 FF                                call    MiProcessLoaderEntry
//	PAGE:000000014049C5D7 8B 05 A3 BC F0 FF                             mov     eax, cs:PerfGlobalGroupMask
//	PAGE:000000014049C5DD A8 04                                         test    al, 4
//	*/
//
//	pAddress = get_undocument_function_address(NULL, pAddress, code2, 9, 0x300, 0x60, 3, FALSE);
//
//	if (pAddress == NULL)
//	{
//		Print("MiProcessLoaderEntry 1 faild!\n");
//		pAddress = get_undocument_function_address(NULL, pMiUnloadSystemImage, code3, 2, 0x300, 0x60, -11, FALSE);
//		DbgBreakPoint();
//		if (pAddress == NULL)
//			return STATUS_UNSUCCESSFUL;
//	}
//
//	g_pfn_mi_process_loader_entry = (mi_process_loader_entry)get_call_point(pAddress);
//
//	if (g_pfn_mi_process_loader_entry == NULL)
//	{
//		Print("MiProcessLoaderEntry 2 faild!\n");
//		return STATUS_UNSUCCESSFUL;
//	}
//
//	//DbgBreakPoint();
//
//	Print("0x%p\n", g_pfnMiProcessLoaderEntry);
//
//	/*////////////////////////////////隐藏驱动/////////////////////////////////*/
//	support_seh(pTargetDriverObject);
//	g_pfn_mi_process_loader_entry(pTargetDriverObject->DriverSection, 0);
//
//	pTargetDriverObject->DriverSection = NULL;
//	/*/////////////////////////////////////////////////////////////////////////*/
//
//	// 破坏驱动对象特征
//	pTargetDriverObject->DriverStart = NULL;
//	pTargetDriverObject->DriverSize = NULL;
//	pTargetDriverObject->DriverUnload = NULL;
//	pTargetDriverObject->DriverInit = NULL;
//	pTargetDriverObject->DeviceObject = NULL;
//
//	return STATUS_SUCCESS;
//}