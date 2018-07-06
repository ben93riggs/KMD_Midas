#include "main.h"

NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	p_driver_object = driver_object;

	RtlInitUnicodeString(&dos, L"\\DosDevices\\midas");
	RtlInitUnicodeString(&dev, L"\\Device\\midas");

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

	return STATUS_SUCCESS;
}

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	if (!clean_unloaded_drivers())
		return STATUS_UNSUCCESSFUL;

	RtlInitUnicodeString(&drv, L"\\Driver\\midas");

	return IoCreateDriver(&drv, &driver_initialize);
}