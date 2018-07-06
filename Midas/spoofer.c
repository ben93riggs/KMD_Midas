#include "spoofer.h"

#define  BUFFER_SIZE 30

CHAR sz_fake_serial[32];
static ULONG randseed = 0;
_declspec(noinline)ULONG __stdcall rand() { return (randseed = randseed * 214013L + 2531011L) >> 16 & 0x7fff; }
_declspec(noinline)VOID __stdcall RandomString(PCHAR buf, INT len) {
	buf[--len] = 0;
	while (--len >= 0)
		buf[len] = (CHAR)('a' + rand() % 25);
}

//NTSTATUS completed_storage_query(PDEVICE_OBJECT device_object, PIRP irp, PVOID context)
//{
//	if (!context)
//	{
//		KdPrint(("%s %d : Context was nullptr\n", __FUNCTION__, __LINE__));
//		return STATUS_SUCCESS;
//	}
//
//	const PREQUEST_STRUCT request = (PREQUEST_STRUCT)context;
//	const ULONG buffer_length = request->OutputBufferLength;
//	const PSTORAGE_DEVICE_DESCRIPTOR buffer = (PSTORAGE_DEVICE_DESCRIPTOR)request->SystemBuffer;
//	const PIO_COMPLETION_ROUTINE old_routine = request->OldRoutine;
//	const PVOID old_context = request->OldContext;
//	ExFreePool(context);
//
//	do
//	{
//		if (buffer_length < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties))
//			break;	// They just want the size
//
//		if (buffer->SerialNumberOffset == 0)
//		{
//			KdPrint(("%s %d : Device doesn't have unique ID\n", __FUNCTION__, __LINE__));
//			break;
//		}
//
//		if (buffer_length < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) + buffer->RawPropertiesLength
//			|| buffer->SerialNumberOffset < FIELD_OFFSET(STORAGE_DEVICE_DESCRIPTOR, RawDeviceProperties) 
//			|| buffer->SerialNumberOffset >= buffer_length
//			)
//		{
//			KdPrint(("%s %d : Malformed buffer (should never happen) size: %d\n", __FUNCTION__, __LINE__, buffer_length));
//		}
//		else
//		{
//			const auto serial = (char*)buffer + buffer->SerialNumberOffset;
//			KdPrint(("%s %d : Serial0: %s\n", __FUNCTION__, __LINE__, serial));
//			spoof_serial(serial, false);
//			KdPrint(("%s %d : Serial1: %s\n", __FUNCTION__, __LINE__, serial));
//		}
//	} while (0);
//
//	// Call next completion routine (if any)
//	if (irp->StackCount > 1ul && old_routine)
//		return old_routine(device_object, irp, old_context);
//
//	return STATUS_SUCCESS;
//}
//
//NTSTATUS completed_smart(PDEVICE_OBJECT device_object, PIRP irp, PVOID context)
//{
//	if (!context)
//	{
//		KdPrint(("%s %d : Context was nullptr\n", __FUNCTION__, __LINE__));
//		return STATUS_SUCCESS;
//	}
//
//	const PREQUEST_STRUCT request = (PREQUEST_STRUCT)context;
//	const ULONG buffer_length = request->OutputBufferLength;
//	const SENDCMDOUTPARAMS* buffer = (SENDCMDOUTPARAMS*)request->SystemBuffer;
//	const PIO_COMPLETION_ROUTINE old_routine = request->OldRoutine;
//	const PVOID old_context = request->OldContext;
//	ExFreePool(context);
//
//	if (buffer_length < FIELD_OFFSET(SENDCMDOUTPARAMS, bBuffer)
//		|| FIELD_OFFSET(SENDCMDOUTPARAMS, bBuffer) + buffer->cBufferSize > buffer_length
//		|| buffer->cBufferSize < sizeof(IDINFO)
//		)
//	{
//		KdPrint(("%s %d : Malformed buffer (should never happen) size: %d\n", __FUNCTION__, __LINE__, buffer_length));
//	}
//	else
//	{
//		const IDINFO* info = (IDINFO*)buffer->bBuffer;
//		const auto serial = info->sSerialNumber;
//		KdPrint(("%s %d : Serial0: %s\n", __FUNCTION__, __LINE__, serial));
//		spoof_serial(serial, true);
//		KdPrint(("%s %d : Serial1: %s\n", __FUNCTION__, __LINE__, serial));
//	}
//
//	// I have no fucking idea why not calling the original doesnt cause problems but whatever
//
//	//KdPrint(("%s: Returning STATUS_NOT_SUPPORTED\n", __FUNCTION__));
//
//	// We deny access by returning an ERROR code
//	//irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
//
//	// Call next completion routine (if any)
//	//if ((irp->StackCount > (ULONG)1) && (OldCompletionRoutine != NULL))
//	//	return OldCompletionRoutine(device_object, irp, OldContext);
//
//	return irp->IoStatus.Status;
//}
//
//void do_completion_hook(PIRP irp, PIO_STACK_LOCATION ioc, PIO_COMPLETION_ROUTINE routine)
//{
//	// Register CompletionRotuine
//	ioc->Control = 0;
//	ioc->Control |= SL_INVOKE_ON_SUCCESS;
//
//	// Save old completion routine
//	// Yes we rewrite any routine to be on success only
//	// and somehow it doesnt cause disaster
//	const auto old_context = ioc->Context;
//	ioc->Context = ExAllocatePool(NonPagedPool, sizeof(REQUEST_STRUCT));
//	const PREQUEST_STRUCT request = (PREQUEST_STRUCT)ioc->Context;
//	request->OldRoutine = ioc->CompletionRoutine;
//	request->OldContext = old_context;
//	request->OutputBufferLength = ioc->Parameters.DeviceIoControl.OutputBufferLength;
//	request->SystemBuffer = irp->AssociatedIrp.SystemBuffer;
//
//	// Setup our function to be called upon completion of the IRP
//	ioc->CompletionRoutine = routine;
//}