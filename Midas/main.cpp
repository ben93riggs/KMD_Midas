#include "main.h"
#include "iocontrol.h"

#define STEALTH_AREA_SIZE 0x20000
STEALTH_DATA_AREA(STEALTH_AREA_SIZE);

NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	VIRTUALIZER_START
	p_driver_object = driver_object;

	RtlInitUnicodeString(&dos, L"\\DosDevices\\trogdor");
	RtlInitUnicodeString(&dev, L"\\Device\\trogdor");

	IoCreateDevice(driver_object, 0, &dev, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &p_device_object);
	IoCreateSymbolicLink(&dos, &dev);

	if (driver_object)
	{
		driver_object->MajorFunction[IRP_MJ_CREATE] = create_call;
		driver_object->MajorFunction[IRP_MJ_CLOSE] = close_call;
		driver_object->MajorFunction[IRP_MJ_DEVICE_CONTROL] = io_control;
		driver_object->DriverUnload = unload_driver;
	}

	p_device_object->Flags |= DO_DIRECT_IO;
	p_device_object->Flags &= ~DO_DEVICE_INITIALIZING;
	VIRTUALIZER_END
	return STATUS_SUCCESS;
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	REFERENCE_STEALTH_DATA_AREA;
	VIRTUALIZER_START
	RtlInitUnicodeString(&drv, L"\\Driver\\trogdor");
	NTSTATUS ret = IoCreateDriver(&drv, &driver_initialize);
	VIRTUALIZER_END

	return ret;
}