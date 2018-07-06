#pragma once
#include "ntos.h"
#include "Structures.h"

NTSTATUS copy_memory(IN PCOPY_MEMORY p_copy);
NTSTATUS get_base_address(IN OUT PBASE_ADDRESS data);
BOOLEAN clean_unloaded_drivers();
//void support_seh(const PDRIVER_OBJECT p_driver_object);
//BOOL _stdcall is_current_process(PCHAR);
//PVOID get_kernel_base(PULONG pImageSize);
//void DISABLE_WP_FLAG();
//void ENABLE_WP_FLAG();