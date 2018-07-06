#pragma once
#define IO_COPYMEM_REQUEST				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x701, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) // Request to read/write virtual user memory (memory of a program) from kernel space
#define IO_GET_ID_REQUEST				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x703, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // Request to retrieve the process id of csgo process, from kernel space
#define IO_GET_MODULE_REQUEST			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x704, METHOD_BUFFERED, FILE_SPECIAL_ACCESS) // Request to retrieve the base address of client.dll in csgo.exe from kernel space
#define IO_SET_PROCID_REQUEST			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x705, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define IO_GET_BASE_ADDRESS				CTL_CODE(FILE_DEVICE_UNKNOWN, 0x706, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)