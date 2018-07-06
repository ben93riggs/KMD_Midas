#pragma once
#include "ntos.h"
#include "utils.h"
#include "hide_driver.h"
#include "IORequests.h"
#include "spoofer.h"
#include "intrin.h"
#include "iocontrol.h"

UNICODE_STRING dev, dos, drv; // Driver registry paths
ULONG procid;
ULONGLONG client_address;
PDEVICE_OBJECT p_device_object;
PDRIVER_OBJECT p_driver_object;

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path); //false entry
NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path); //real entry