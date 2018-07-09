#pragma once
#include "StealthDataArea.h"
#include "VirtualizerSDK.h"
#include "ntos.h"

UNICODE_STRING dev, dos, drv; // Driver registry paths
PDEVICE_OBJECT p_device_object;
PDRIVER_OBJECT p_driver_object;

NTSTATUS driver_entry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path); //false entry
NTSTATUS driver_initialize(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path); //real entry