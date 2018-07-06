#pragma once
#include "ntos.h"

//-------------------------------------------------------------------------------------------------
#define IOCTL_STORAGE_BASE 0x0000002d
#define IOCTL_DISK_BASE 0x00000007
#define ATA_IDENTIFY_DEVICE 0xec

#define IOCTL_STORAGE_QUERY_PROPERTY	CTL_CODE(IOCTL_STORAGE_BASE,0x0500,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define SMART_RCV_DRIVE_DATA			CTL_CODE(IOCTL_DISK_BASE,0x0022,METHOD_BUFFERED,FILE_READ_ACCESS|FILE_WRITE_ACCESS)

struct ata_identify_device
{
	USHORT words000_009[10];
	UCHAR serial_no[20];
	USHORT words020_022[3];
	UCHAR fw_rev[8];
	UCHAR model[40];
	USHORT words047_079[33];
	USHORT major_rev_num;
	USHORT minor_rev_num;
	USHORT command_set_1;
	USHORT command_set_2;
	USHORT command_set_extension;
	USHORT cfs_enable_1;
	USHORT word086;
	USHORT csf_default;
	USHORT words088_255[168];
};

typedef enum _STORAGE_BUS_TYPE
{
	BusTypeUnknown = 0x00,
	BusTypeScsi,
	BusTypeAtapi,
	BusTypeAta,
	BusType1394,
	BusTypeSsa,
	BusTypeFibre,
	BusTypeUsb,
	BusTypeRAID,
	BusTypeMaxReserved = 0x7F
}STORAGE_BUS_TYPE, *PSTORAGE_BUS_TYPE;

// retrieve the storage device descriptor data for a device.
typedef struct _STORAGE_DEVICE_DESCRIPTOR
{
	ULONG  Version;
	ULONG  Size;
	UCHAR  DeviceType;
	UCHAR  DeviceTypeModifier;
	BOOLEAN  RemovableMedia;
	BOOLEAN  CommandQueueing;
	ULONG  VendorIdOffset;
	ULONG  ProductIdOffset;
	ULONG  ProductRevisionOffset;
	ULONG  SerialNumberOffset;
	STORAGE_BUS_TYPE  BusType;
	ULONG  RawPropertiesLength;
	UCHAR  RawDeviceProperties[1];
}STORAGE_DEVICE_DESCRIPTOR, *PSTORAGE_DEVICE_DESCRIPTOR;

typedef struct _GETVERSIONOUTPARAMS
{
	UCHAR  bVersion;
	UCHAR  bRevision;
	UCHAR  bReserved;
	UCHAR  bIDEDeviceMap;
	ULONG  fCapabilities;
	ULONG  dwReserved[4];
}GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

typedef struct _IDEREGS
{
	UCHAR  bFeaturesReg;
	UCHAR  bSectorCountReg;
	UCHAR  bSectorNumberReg;
	UCHAR  bCylLowReg;
	UCHAR  bCylHighReg;
	UCHAR  bDriveHeadReg;
	UCHAR  bCommandReg;
	UCHAR  bReserved;
}IDEREGS, *PIDEREGS, *LPIDEREGS;

typedef struct _SENDCMDINPARAMS
{
	ULONG  cBufferSize;
	IDEREGS  irDriveRegs;
	UCHAR  bDriveNumber;
	UCHAR  bReserved[3];
	ULONG  dwReserved[4];
	UCHAR  bBuffer[1];
}SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;

typedef struct _DRIVERSTATUS
{
	UCHAR  bDriverError;
	UCHAR  bIDEError;
	UCHAR  bReserved[2];
	ULONG  dwReserved[2];
}DRIVERSTATUS, *PDRIVERSTATUS, *LPDRIVERSTATUS;

typedef struct _SENDCMDOUTPARAMS
{
	ULONG  cBufferSize;
	DRIVERSTATUS  DriverStatus;
	UCHAR  bBuffer[1];
}SENDCMDOUTPARAMS, *PSENDCMDOUTPARAMS, *LPSENDCMDOUTPARAMS;
//-------------------------------------------------------------------------------------------------
