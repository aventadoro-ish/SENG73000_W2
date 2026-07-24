#pragma once
#ifndef PCAN_PROXY
#define PCAN_PROXY

#ifdef DO_USE_CAN
#error "pcan_proxy.h should not be used if DO_USE_CAN is set."
#endif

// define proxy for the datatypes used by the library

// compatibilty defines
#if defined(DWORD) || defined(WORD) || defined(BYTE)
#error "double define for DWORD, WORD, BYTE found"
#endif


#include <cstdint>

#define DWORD  uint32_t
#define WORD   uint16_t
#define BYTE   uint8_t


#if (defined (_WIN32) || defined (_WIN64))
    // windows code
    // https://stackoverflow.com/a/19365248
    #define O_RDONLY         00
    #define O_WRONLY         01
    #define O_RDWR           02
#elif (defined (LINUX) || defined (__linux__))
#include <fcntl.h>    					// O_RDWR
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#error "Unsupported OS"
#endif











/******************************************************************************
 *                              pcan.h definitions
 ******************************************************************************/
#ifndef __PCAN_H__
#define __PCAN_H__

//****************************************************************************
// Copyright (C) 2001-2010  PEAK System-Technik GmbH
//
// linux@peak-system.com 
// www.peak-system.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// Maintainer(s): Klaus Hitschler (klaus.hitschler@gmx.de)
//****************************************************************************

//****************************************************************************
//
// pcan.h
// constants and definitions to access the drivers 
//
// $Id: pcan.h 615 2010-02-14 22:38:55Z khitschler $
//
//****************************************************************************

//****************************************************************************

//****************************************************************************
// parameter wHardwareType, used by open 
#define HW_ISA             1 // not supported with LINUX, 82C200 chip 
#define HW_DONGLE_SJA      5
#define HW_DONGLE_SJA_EPP  6 
#define HW_DONGLE_PRO      7 // not yet supported with LINUX
#define HW_DONGLE_PRO_EPP  8 // not yet supported with LINUX
#define HW_ISA_SJA         9 // use this also for PC/104
#define HW_PCI            10 // PCI carries always SJA1000 chips
#define HW_USB            11 // don't know if this is common over peak products
#define HW_PCCARD         12 // not aligned to other OS
#define HW_USB_PRO        13 

//****************************************************************************
// mask for standard and extended CAN identifiers
#define CAN_MAX_STANDARD_ID     0x7ff
#define CAN_MAX_EXTENDED_ID     0x1fffffff

//****************************************************************************
// error codes
#define CAN_ERR_OK             0x0000  // no error
#define CAN_ERR_XMTFULL        0x0001  // transmit buffer full
#define CAN_ERR_OVERRUN        0x0002  // overrun in receive buffer
#define CAN_ERR_BUSLIGHT       0x0004  // bus error, errorcounter limit reached
#define CAN_ERR_BUSHEAVY       0x0008  // bus error, errorcounter limit reached
#define CAN_ERR_BUSOFF         0x0010  // bus error, 'bus off' state entered
#define CAN_ERR_QRCVEMPTY      0x0020  // receive queue is empty
#define CAN_ERR_QOVERRUN       0x0040  // receive queue overrun
#define CAN_ERR_QXMTFULL       0x0080  // transmit queue full 
#define CAN_ERR_REGTEST        0x0100  // test of controller registers failed
#define CAN_ERR_NOVXD          0x0200  // Win95/98/ME only
#define CAN_ERR_RESOURCE       0x2000  // can't create resource
#define CAN_ERR_ILLPARAMTYPE   0x4000  // illegal parameter
#define CAN_ERR_ILLPARAMVAL    0x8000  // value out of range
#define CAN_ERRMASK_ILLHANDLE  0x1C00  // wrong handle, handle error

//****************************************************************************
// MSGTYPE bits of element MSGTYPE in structure TPCANMsg
#define MSGTYPE_STATUS        0x80     // used to mark a status TPCANMsg
#define MSGTYPE_EXTENDED      0x02     // declares a extended frame
#define MSGTYPE_RTR           0x01     // marks a remote frame
#define MSGTYPE_STANDARD      0x00     // marks a standard frame

//****************************************************************************
// maximum length of the version string (attention: used in driver too)
#define VERSIONSTRING_LEN     64  

//****************************************************************************
// structures to communicate via ioctls
typedef struct
{
  WORD wBTR0BTR1;        // merged BTR0 and BTR1 register of the SJA1000
  BYTE ucCANMsgType;     // 11 or 29 bits - put MSGTYPE_... in here
  BYTE ucListenOnly;     // listen only mode when != 0
} TPCANInit;             // for PCAN_INIT

typedef struct 
{
  DWORD ID;              // 11/29 bit code
  BYTE  MSGTYPE;         // bits of MSGTYPE_*
  BYTE  LEN;             // count of data bytes (0..8)
  BYTE  DATA[8];         // data bytes, up to 8
} TPCANMsg;              // for PCAN_WRITE_MSG

typedef struct
{
  TPCANMsg Msg;          // the above message
  DWORD    dwTime;       // a timestamp in msec, read only
  WORD     wUsec;        // remainder in micro-seconds
} TPCANRdMsg;            // for PCAN_READ_MSG

typedef struct 
{
  WORD  wErrorFlag;      // same as in TPDIAG, is cleared in driver after access
  int   nLastError;      // is cleared in driver after access
} TPSTATUS;              // for PCAN_GET_STATUS

typedef struct
{
  WORD  wType;           // the type of interface hardware - see HW_....
  DWORD dwBase;          // the base address or port of this device
  WORD  wIrqLevel;       // the irq level of this device
  DWORD dwReadCounter;   // counts all reads to this device from start
  DWORD dwWriteCounter;  // counts all writes
  DWORD dwIRQcounter;    // counts all interrupts
  DWORD dwErrorCounter;  // counts all errors
  WORD  wErrorFlag;      // gathers all errors
  int   nLastError;      // the last local error for this device
  int   nOpenPaths;      // number of open paths for this device
  char  szVersionString[VERSIONSTRING_LEN]; // driver version string
} TPDIAG;                // for PCAN_DIAG, in opposition to PCAN_GET_STATUS nothing is cleared
  
typedef struct
{
  DWORD dwBitRate;       // in + out, bitrate in bits per second
  WORD  wBTR0BTR1;       // out only: the result
} TPBTR0BTR1;

typedef struct 
{
  WORD  wErrorFlag;      // same as in TPDIAG, is cleared in driver after access
  int   nLastError;      // is cleared in driver after access
  int   nPendingReads;   // count of unread telegrams
  int   nPendingWrites;  // count of unsent telegrams
} TPEXTENDEDSTATUS;      // for PCAN_GET_ESTATUS

typedef struct
{
  DWORD FromID;          // First CAN ID to accept
  DWORD ToID;            // Last CAN ID to accept
  BYTE  MSGTYPE;         // bits of MSGTYPE_*
} TPMSGFILTER;

//****************************************************************************
// currently available sub-functions
#define SF_GET_SERIALNUMBER 1 // to get the serial number (currently only pcan-usb)
#define SF_SET_SERIALNUMBER 2 // to set the serial number (currently only pcan-usb)
#define SF_GET_HCDEVICENO   3 // request hardcoded device number (currently only pcan-usb)
#define SF_SET_HCDEVICENO   4 // to set hardcoded device number (currently only pcan-usb)

typedef struct
{
  int   nSubFunction;    // a sub-function number SF_... to determine the union element used
  union
  {
    DWORD dwSerialNumber; // to get and set the pcan-usb serial number
    BYTE  ucHCDeviceNo;   // only for USB-devices to get or set a hard assigned number
  } func;
} TPEXTRAPARAMS;

//****************************************************************************
// some predefines for ioctls
#define PCAN_MAGIC_NUMBER  'z'
#define MYSEQ_START        0x80

//****************************************************************************
// ioctls control codes
#define PCAN_INIT           _IOWR(PCAN_MAGIC_NUMBER, MYSEQ_START,     TPCANInit)
#define PCAN_WRITE_MSG      _IOW (PCAN_MAGIC_NUMBER, MYSEQ_START + 1, TPCANMsg)
#define PCAN_READ_MSG       _IOR (PCAN_MAGIC_NUMBER, MYSEQ_START + 2, TPCANRdMsg)
#define PCAN_GET_STATUS     _IOR (PCAN_MAGIC_NUMBER, MYSEQ_START + 3, TPSTATUS)
#define PCAN_DIAG           _IOR (PCAN_MAGIC_NUMBER, MYSEQ_START + 4, TPDIAG)
#define PCAN_BTR0BTR1       _IOWR(PCAN_MAGIC_NUMBER, MYSEQ_START + 5, TPBTR0BTR1)
#define PCAN_GET_EXT_STATUS _IOR (PCAN_MAGIC_NUMBER, MYSEQ_START + 6, TPEXTENDEDSTATUS)
#define PCAN_MSG_FILTER     _IOW (PCAN_MAGIC_NUMBER, MYSEQ_START + 7, TPMSGFILTER)
#define PCAN_EXTRA_PARAMS   _IOWR(PCAN_MAGIC_NUMBER, MYSEQ_START + 8, TPEXTRAPARAMS)

#endif // __PCAN_H__















/******************************************************************************
 *                              libpcan.h definitions
 ******************************************************************************/

#ifndef __LIBPCAN_H__
#define __LIBPCAN_H__

//****************************************************************************
// Copyright (C) 2001-2007  PEAK System-Technik GmbH
//
// linux@peak-system.com
// www.peak-system.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Maintainer(s): Klaus Hitschler (klaus.hitschler@gmx.de)
// Contributions: Mudiaga Obada   (obada@vsi.cs.uni-frankfurt.de)
//****************************************************************************

//****************************************************************************
//
// libpcan.h
// common header to access the functions within pcanlib.so.x.x,
// originally created from Wilhelm Hoppe in pcan_pci.h
//
// $Id: libpcan.h 455 2007-02-11 22:19:11Z khitschler $
//
//****************************************************************************

//****************************************************************************
//****************************************************************************

// compatibilty defines
#if defined(LPSTR) || defined(HANDLE)
#error "double define for LPSTR, HANDLE found"
#endif

#define LPSTR  char *
#define HANDLE void *

//****************************************************************************
// for CAN_Open(...)

//****************************************************************************
// for CAN_Init(...)

// parameter wBTR0BTR1
// bitrate codes of BTR0/BTR1 registers
#define CAN_BAUD_1M     0x0014  //   1 MBit/s
#define CAN_BAUD_500K   0x001C  // 500 kBit/s
#define CAN_BAUD_250K   0x011C  // 250 kBit/s
#define CAN_BAUD_125K   0x031C  // 125 kBit/s
#define CAN_BAUD_100K   0x432F  // 100 kBit/s
#define CAN_BAUD_50K    0x472F  //  50 kBit/s
#define CAN_BAUD_20K    0x532F  //  20 kBit/s
#define CAN_BAUD_10K    0x672F  //  10 kBit/s
#define CAN_BAUD_5K     0x7F7F  //   5 kBit/s

// parameter nCANMsgType
#define CAN_INIT_TYPE_EX		0x01	//Extended Frame
#define CAN_INIT_TYPE_ST		0x00	//Standart Frame

//****************************************************************************
// error codes are defined in pcan.h
#define CAN_ERR_ANYBUSERR (CAN_ERR_BUSLIGHT | CAN_ERR_BUSHEAVY | CAN_ERR_BUSOFF)

//****************************************************************************
// PROTOTYPES
#ifdef __cplusplus
  extern "C"
{
#endif

//****************************************************************************
//  CAN_Open()
//  creates a path to a CAN port
//
//  for PCAN-Dongle call:             CAN_Open(HW_DONGLE_.., DWORD dwPort, WORD wIrq);
//  for PCAN-ISA or PCAN-PC/104 call: CAN_Open(HW_ISA_SJA, DWORD dwPort, WORD wIrq);
//  for PCAN-PCI call:                CAN_Open(HW_PCI, int nPort); .. enumerate nPort 1..8.
//
//  if ((dwPort == 0) && (wIrq == 0)) CAN_Open() takes the 1st default ISA or DONGLE port.
//  if (nPort == 0) CAN_Open() takes the 1st default PCI port.
//  returns NULL when open failes.
//
//  The first CAN_Open() to a CAN hardware initializes the hardware to default
//  parameter 500 kbit/sec and acceptance of extended frames.
//
HANDLE CAN_Open(WORD wHardwareType, ...);

//****************************************************************************
//  CAN_Init()
//  initializes the CAN hardware with the BTR0 + BTR1 constant "CAN_BAUD_...".
//  nCANMsgType must be filled with "CAN_INIT_TYPE_..".
//  The default initialisation, e.g. CAN_Init is not called,
//  is 500 kbit/sec and extended frames.
//
DWORD CAN_Init(HANDLE hHandle, WORD wBTR0BTR1, int nCANMsgType);

//****************************************************************************
//  CAN_Close()
//  closes the path to the CAN hardware.
//  The last close on the hardware put the chip into passive state.
DWORD CAN_Close(HANDLE hHandle);

//****************************************************************************
//  CAN_Status()
//  request the current (stored) status of the CAN hardware. After the read the
//  stored status is reset.
//  If the status is negative a system error is returned (e.g. -EBADF).
DWORD CAN_Status(HANDLE hHandle);

//****************************************************************************
//  CAN_Write()
//  writes a message to the CAN bus. If the write queue is full the current
//  write blocks until either a message is sent or a error occured.
//
DWORD CAN_Write(HANDLE hHandle, TPCANMsg* pMsgBuff);

//****************************************************************************
//  LINUX_CAN_Write_Timeout()
//  writes a message to the CAN bus. If the (software) message buffer is full
//  the current write request blocks until a write slot gets empty 
//  or a timeout or a error occures.
//  nMicroSeconds  > 0 -> Timeout in microseconds
//  nMicroSeconds == 0 -> polling
//  nMicroSeconds  < 0 -> blocking, same as CAN_Write()
DWORD LINUX_CAN_Write_Timeout(HANDLE hHandle, TPCANMsg* pMsgBuff, int nMicroSeconds);

//****************************************************************************
//  CAN_Read()
//  reads a message from the CAN bus. If there is no message to read the current
//  request blocks until either a new message arrives or a error occures.
DWORD CAN_Read(HANDLE hHandle, TPCANMsg* pMsgBuff);

//****************************************************************************
//  LINUX_CAN_Read()
//  reads a message WITH TIMESTAMP from the CAN bus. If there is no message 
//  to read the current request blocks until either a new message arrives 
//  or a error occures.
DWORD LINUX_CAN_Read(HANDLE hHandle, TPCANRdMsg* pMsgBuff);

//****************************************************************************
//  LINUX_CAN_Read_Timeout()
//  reads a message WITH TIMESTAMP from the CAN bus. If there is no message 
//  to read the current request blocks until either a new message arrives 
//  or a timeout or a error occures.
//  nMicroSeconds  > 0 -> Timeout in microseconds
//  nMicroSeconds == 0 -> polling
//  nMicroSeconds  < 0 -> blocking, same as LINUX_CAN_Read()
DWORD LINUX_CAN_Read_Timeout(HANDLE hHandle, TPCANRdMsg* pMsgBuff, int nMicroSeconds);

//***************************************************************************
// CAN_ResetFilter() - removes all current Message Filters
// Caution! Currently this operation influences all read paths
//
DWORD CAN_ResetFilter(HANDLE hHandle);

//***************************************************************************
// CAN_MsgFilter() - reduce received data in to FromID <= ID <= ToID
// Type may be MSGTYPE_STANDARD or MSGTYPE_EXTENDED
// This function can be called multiple to add more ranges.
// Caution! Currently this operation influences all read paths
//
DWORD CAN_MsgFilter(HANDLE hHandle, DWORD FromID, DWORD ToID, int nCANMsgType);

//***************************************************************************
// LINUX_CAN_FileHandle() - return PCAN driver file handle for select(2)
//
int LINUX_CAN_FileHandle(HANDLE hHandle);

//****************************************************************************
//  LINUX_CAN_Extended_Status()
//  get the same as CAN_Status() with additional informaton about pending reads or writes
//
//  There is a uncertainty of 1 message for "nPendingWrites" for a small amount 
//  of time between the messages is put into the CAN sender and the telegram is
//  successfuly sent or an error is thrown.
DWORD LINUX_CAN_Extended_Status(HANDLE hHandle, int *nPendingReads, int *nPendingWrites);

//****************************************************************************
//  CAN_VersionInfo()
//  returns a text string with driver version info.
//
DWORD CAN_VersionInfo(HANDLE hHandle, LPSTR lpszTextBuff);

//****************************************************************************
//  nGetLastError()
//  returns the last stored error (errno of the shared library). The returend
//  error is independend of any path.
//
int nGetLastError(void);

//****************************************************************************
//  LINUX_CAN_Open() - another open, LINUX like
//  creates a path to a CAN port
//
//  input: the path to the device node (e.g. /dev/pcan0)
//  returns NULL when open failes
//
HANDLE LINUX_CAN_Open(const char *szDeviceName, int nFlag);

//****************************************************************************
//  LINUX_CAN_Statistics() - get statistics about this devices
//
DWORD LINUX_CAN_Statistics(HANDLE hHandle, TPDIAG *diag);

//****************************************************************************
//  LINUX_CAN_BTR0BTR1() - get the BTR0 and BTR1 from bitrate, LINUX like
//
//  input:  the handle to the device node
//          the bitrate in bits / second, e.g. 500000 bits/sec
//
//  returns 0 if not possible
//          BTR0BTR1 for the interface
//
WORD LINUX_CAN_BTR0BTR1(HANDLE hHandle, DWORD dwBitRate);

#ifdef __cplusplus
}
#endif
#endif // __LIBPCAN_H__



#endif