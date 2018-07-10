#include "main.h"
#include "iocontrol.h"
#include "Structures.h"
#include "utils.h"
#include "IORequests.h"

//extern "C" NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
//{
//	
//	p_driver_object = driver_object;
//
//	RtlInitUnicodeString(&dos, L"\\DosDevices\\midas");
//	RtlInitUnicodeString(&dev, L"\\Device\\midas");
//
//	IoCreateDevice(driver_object, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &p_device_object);
//	IoCreateSymbolicLink(&dos, &dev);
//
//	if (driver_object)
//	{
//		driver_object->MajorFunction[IRP_MJ_CREATE] = create_call;
//		driver_object->MajorFunction[IRP_MJ_CLOSE] = close_call;
//		driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = io_control;
//		driver_object->DriverUnload = unload_driver;
//	}
//
//	p_device_object->Flags |= DO_DIRECT_IO;
//	p_device_object->Flags &= ~DO_DEVICE_INITIALIZING;
//	
//	return STATUS_SUCCESS;
//}

namespace original
{
	PDRIVER_OBJECT driver_object = nullptr;
	PDRIVER_UNLOAD unload = nullptr;
	PDRIVER_DISPATCH major_functions[IRP_MJ_MAXIMUM_FUNCTION + 1] = { nullptr };
	PDEVICE_OBJECT device = nullptr;
	BOOLEAN destroy_device = FALSE;
	ULONGLONG guard_icall = 0;
}

extern "C" NTSTATUS CreateSpoofedDevice(_In_ struct _DRIVER_OBJECT * driver, _Out_ PDEVICE_OBJECT* device)
{
	if (driver->DeviceObject != nullptr)
		return STATUS_DEVICE_ALREADY_ATTACHED;

	*device = nullptr;

	UNICODE_STRING device_name{}, dos_device_name{};

	RtlInitUnicodeString(&device_name, L"\\Device\\midas");
	RtlInitUnicodeString(&dos_device_name, L"\\DosDevices\\midas");

	auto status = IoCreateDevice(driver, 0, &device_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, device);

	if (NT_ERROR(status)) {
		Print("Failed to create a spoofed device. Skipping.\n");

		*device = nullptr;
		return status;
	}

	status = IoCreateSymbolicLink(&dos_device_name, &device_name);

	if (NT_ERROR(status))
	{
		Print("Failed to create symlink for spoofed device. Skipping.\n");

		IoDeleteDevice(*device);
		*device = nullptr;
		return status;
	}

	// Finish off initialization by setting flags
	(*device)->Flags &= ~DO_DEVICE_INITIALIZING;
	(*device)->Flags |= DO_BUFFERED_IO;

	return STATUS_SUCCESS;
}

extern "C" VOID DestroyDevice(PDEVICE_OBJECT* device)
{
	if (*device == nullptr)
		return;

	IoDeleteDevice(*device);
	*device = nullptr;
}

extern "C" NTSTATUS DeleteSymLink()
{
	UNICODE_STRING dos_device_name{};
	RtlInitUnicodeString(&dos_device_name, L"\\DosDevices\\midas");
	return IoDeleteSymbolicLink(&dos_device_name);
}

extern "C" NTSTATUS CreateSymLink(PDEVICE_OBJECT device)
{
	ULONG size = 0;
	PUNICODE_STRING device_name = nullptr;
	auto status = STATUS_SUCCESS;
	DbgPrint("Creating SymLink!\n");
	status = ObQueryNameString(device, nullptr, 0, &size);

	if (status == STATUS_INFO_LENGTH_MISMATCH) {

		device_name = PUNICODE_STRING(ExAllocatePoolWithTag(NonPagedPool, size, 'blah'));

		if (device_name != nullptr) {

			RtlSecureZeroMemory(device_name, size);
			status = ObQueryNameString(device, POBJECT_NAME_INFORMATION(device_name), size, &size);
			if (NT_SUCCESS(status) && device_name->Buffer != nullptr)
			{
				UNICODE_STRING dos_device_name{};
				RtlInitUnicodeString(&dos_device_name, L"\\DosDevices\\midas");

				status = IoCreateSymbolicLink(&dos_device_name, device_name);
			}
			else
			{
				status = STATUS_INTERNAL_ERROR;
			}
		}

		ExFreePoolWithTag(device_name, 'blah');
	}

	return status;
}

extern "C" VOID RestoreDriver()
{
	if (original::driver_object == nullptr)
		return;

	if (original::unload != nullptr)
	{
		original::driver_object->DriverUnload = original::unload;
		original::unload = nullptr;
	}

	// restore irp handlers
	if (NT_ERROR(CopyMajorFunctions(original::major_functions, original::driver_object->MajorFunction, IRP_MJ_MAXIMUM_FUNCTION + 1)))
	{
		// nothing we can really do here tbf
	}

	// re-enable cf guard
	SetCfgDispatch(original::driver_object, original::guard_icall);

	if (original::destroy_device == TRUE)
		DestroyDevice(&original::device);
	original::destroy_device = FALSE;

	DeleteSymLink();

	original::driver_object = nullptr;
}

#pragma region hooks
void UnloadDriver(PDRIVER_OBJECT)
{

}

extern "C" void DispatchUnload(_In_ struct _DRIVER_OBJECT * driver)
{
	UnloadDriver(driver);
	RestoreDriver();
	return driver->DriverUnload(driver);
}

extern "C" NTSTATUS CallOriginal(const int idx, _In_ struct _DEVICE_OBJECT *DeviceObject, _Inout_ struct _IRP *Irp)
{
	if (original::destroy_device == TRUE)
		return STATUS_SUCCESS;

	const auto& function = original::major_functions[idx];

	if (function == nullptr)
		return STATUS_SUCCESS;

	return function(DeviceObject, Irp);
}

extern "C" NTSTATUS CatchCreate(PDEVICE_OBJECT device, PIRP irp)
{
	// TODO: Wipe INIT section on first IRP ;)

	
	//irp->IoStatus.Status = STATUS_SUCCESS;
	//irp->IoStatus.Information = 0;

	//IofCompleteRequest(irp, IO_NO_INCREMENT);
	

	return CallOriginal(IRP_MJ_CREATE, device, irp);
}

extern "C" NTSTATUS CatchClose(PDEVICE_OBJECT device, PIRP irp)
{
	
	//irp->IoStatus.Status = STATUS_SUCCESS;
	//irp->IoStatus.Information = 0;

	//IoCompleteRequest(irp, IO_NO_INCREMENT);
	

	return CallOriginal(IRP_MJ_CLOSE, device, irp);
}

extern "C" NTSTATUS CatchDeviceCtrl(PDEVICE_OBJECT device, PIRP irp)
{
	ULONG bytes_io = 0;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);

	// Code received from user space
	const ULONG control_code = stack->Parameters.DeviceIoControl.IoControlCode;
	const void* io_buffer = irp->AssociatedIrp.SystemBuffer;
	const ULONG input_buffer_length = stack->Parameters.DeviceIoControl.InputBufferLength;

	if (control_code == IO_COPYMEM_REQUEST)
	{
		if (input_buffer_length >= sizeof(COPY_MEMORY) && io_buffer)
			irp->IoStatus.Status = copy_memory((PCOPY_MEMORY)io_buffer);
		else irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

		bytes_io = sizeof(COPY_MEMORY);

		// Complete the request
		irp->IoStatus.Information = bytes_io;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	if (control_code == IO_GET_BASE_ADDRESS)
	{
		ULONGLONG base_address_out = 0;
		if (input_buffer_length >= sizeof(BASE_ADDRESS) && io_buffer)
			irp->IoStatus.Status = get_base_address((PBASE_ADDRESS)io_buffer);
		else
			irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

		bytes_io = sizeof(BASE_ADDRESS);

		// Complete the request
		irp->IoStatus.Information = bytes_io;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	if (control_code == IO_CLEAN_UNLOADED_DRIVERS)
	{
		if (clean_unloaded_drivers())
			irp->IoStatus.Status = STATUS_SUCCESS;
		else
			irp->IoStatus.Status = STATUS_UNSUCCESSFUL;

		bytes_io = 0;

		// Complete the request
		irp->IoStatus.Information = bytes_io;
		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}

	return CallOriginal(IRP_MJ_DEVICE_CONTROL, device, irp);
}
#pragma endregion

extern "C" NTSTATUS hijack_driver(_In_ struct _DRIVER_OBJECT * driver)
{
	// create device
	if (driver->DeviceObject == nullptr)
	{
		const auto status = CreateSpoofedDevice(driver, &original::device);

		if (NT_ERROR(status))
			return status;

		original::destroy_device = TRUE;
	}
	else
	{
		const auto device_name_info = ObQueryNameInfo(driver->DeviceObject);

		if (device_name_info == nullptr)
		{
			DbgPrint("Unnamed device. Skipping.\n");
			return STATUS_NOT_IMPLEMENTED;
		}

		// cf guard fucks you over if you try to hijack existing devices
		original::guard_icall = SetCfgDispatch(driver, ULONGLONG(_ignore_icall));
		original::destroy_device = FALSE;
	}

	original::device = driver->DeviceObject;

	// backup irp handler to call original/ restore them later
	if (NT_ERROR(CopyMajorFunctions(driver->MajorFunction, original::major_functions, IRP_MJ_MAXIMUM_FUNCTION + 1)))
	{
		if (original::destroy_device == TRUE)
			DestroyDevice(&original::device);
		original::destroy_device = FALSE;
		return STATUS_COPY_PROTECTION_FAILURE;
	}

	// replace irp handlers
	driver->MajorFunction[IRP_MJ_CREATE] = &CatchCreate;
	driver->MajorFunction[IRP_MJ_CLOSE] = &CatchClose;
	driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = &CatchDeviceCtrl;

	original::driver_object = driver;

	CreateSymLink(original::device);

	// Windows interprets no unload routine as can't be unloaded so it wouldn't be benefitial to add an unload routine to a driver that doesn't support it.
	//if (driver->DriverUnload != nullptr) {
	//	original::unload = driver->DriverUnload;
	//	driver->DriverUnload = &DispatchUnload;
	//}

	Print("\nhijack success!\n");
	return STATUS_SUCCESS;
}

extern "C" NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	Print("\n\nDRIVER_ENTRY\n\n\n");

	HANDLE handle{};
	OBJECT_ATTRIBUTES attributes{};
	UNICODE_STRING directory_name{};
	PVOID directory{};
	BOOLEAN success = FALSE;

	RtlInitUnicodeString(&directory_name, L"\\Driver");
	InitializeObjectAttributes(&attributes, &directory_name, OBJ_CASE_INSENSITIVE, NULL, NULL);

	// open OBJECT_DIRECTORY for \Driver
	auto status = ZwOpenDirectoryObject(&handle, DIRECTORY_ALL_ACCESS, &attributes);

	if (!NT_SUCCESS(status))
		return status;

	// Get OBJECT_DIRECTORY pointer from HANDLE
	status = ObReferenceObjectByHandle(handle, DIRECTORY_ALL_ACCESS, nullptr, KernelMode, &directory, nullptr);

	if (!NT_SUCCESS(status)) {
		ZwClose(handle);
		return status;
	}

	const auto directory_object = POBJECT_DIRECTORY(directory);

	ExAcquirePushLockExclusiveEx(&directory_object->Lock, 0);

	// traverse hash table with 37 entries
	// when a new object is created, the object manager computes a hash value in the range zero to 36 from the object name and creates an OBJECT_DIRECTORY_ENTRY.    
	// http://www.informit.com/articles/article.aspx?p=22443&seqNum=7
	for (auto entry : directory_object->HashBuckets)
	{
		Print("\nENTRY_OBJECT: %p\n", entry->Object);

		if (entry == nullptr)
			continue;

		if (success == TRUE)
			break;

		while (entry != nullptr && entry->Object != nullptr)
		{
			// You could add type checking here with ObGetObjectType but if that's wrong we're gonna bsod anyway :P
			const auto driver = PDRIVER_OBJECT(entry->Object);

			if (NT_SUCCESS(hijack_driver(driver)))
			{
				success = TRUE;
				break;
			}

			entry = entry->ChainLink;
		}
	}

	ExReleasePushLockExclusiveEx(&directory_object->Lock, 0);

	// Release the acquired resources back to the OS
	ObDereferenceObject(directory);
	ZwClose(handle);

	return success == TRUE ? STATUS_SUCCESS : STATUS_NOT_FOUND;
}