#pragma once

NTSTATUS io_control(PDEVICE_OBJECT device_object, PIRP irp);
NTSTATUS unload_driver(PDRIVER_OBJECT driver_object);
NTSTATUS create_call(PDEVICE_OBJECT device_object, PIRP irp);
NTSTATUS close_call(PDEVICE_OBJECT device_object, PIRP irp);