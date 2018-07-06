#include "main.h"

// IOCTL Call Handler function
NTSTATUS io_control(PDEVICE_OBJECT device_object, PIRP irp)
{
	ULONG bytes_io = 0;
	const PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);

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
	}
	else if (control_code == IO_GET_ID_REQUEST)
	{
		PULONG OutPut = (PULONG)irp->AssociatedIrp.SystemBuffer;
		*OutPut = procid;

		irp->IoStatus.Status = STATUS_SUCCESS;
		bytes_io = sizeof OutPut;
	}
	else if (control_code == IO_GET_MODULE_REQUEST)
	{
		PULONG OutPut = (PULONG)irp->AssociatedIrp.SystemBuffer;
		*OutPut = client_address;

		irp->IoStatus.Status = STATUS_SUCCESS;
		bytes_io = sizeof OutPut;
	}
	else if (control_code == IO_GET_BASE_ADDRESS)
	{
		ULONGLONG base_address_out = 0;
		if (input_buffer_length >= sizeof(BASE_ADDRESS) && io_buffer)
			irp->IoStatus.Status = get_base_address((PBASE_ADDRESS)io_buffer);
		else
			irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

		bytes_io = sizeof(BASE_ADDRESS);
	}
	else
	{
		irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		bytes_io = 0;
	}

	// Complete the request
	irp->IoStatus.Information = bytes_io;
	IoCompleteRequest(irp, IO_NO_INCREMENT);

	return irp->IoStatus.Status;
}

NTSTATUS unload_driver(PDRIVER_OBJECT driver_object)
{
	IoDeleteSymbolicLink(&dos);
	IoDeleteDevice(driver_object->DeviceObject);
	return 0;
}

NTSTATUS create_call(PDEVICE_OBJECT device_object, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS close_call(PDEVICE_OBJECT device_object, PIRP irp)
{
	irp->IoStatus.Status = STATUS_SUCCESS;
	irp->IoStatus.Information = 0;

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}