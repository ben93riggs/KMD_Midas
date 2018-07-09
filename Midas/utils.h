#pragma once
#include "ntos.h"
#include "Structures.h"

BOOLEAN clean_unloaded_drivers();
NTSTATUS copy_memory(IN PCOPY_MEMORY p_copy);
NTSTATUS get_base_address(IN OUT PBASE_ADDRESS data);
