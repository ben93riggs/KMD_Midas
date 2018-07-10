#pragma once
#include "ntos.h"
#include "Structures.h"

extern "C" BOOLEAN clean_unloaded_drivers();
extern "C" NTSTATUS copy_memory(IN PCOPY_MEMORY p_copy);
extern "C" NTSTATUS get_base_address(IN OUT PBASE_ADDRESS data);

extern "C" NTSTATUS CopyMajorFunctions(_In_reads_bytes_(count * sizeof(PDRIVER_DISPATCH)) PDRIVER_DISPATCH* src, _Out_writes_bytes_all_(count * sizeof(PDRIVER_DISPATCH)) PDRIVER_DISPATCH* dst, SIZE_T size);
extern "C" ULONGLONG SetCfgDispatch(PDRIVER_OBJECT driver, ULONGLONG new_dispatch);