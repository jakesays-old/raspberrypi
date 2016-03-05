#ifndef I2C_H
#define I2C_H

#include <stdint.h>

#define PI_NUM_I2C_BUS 2
#define PI_MAX_I2C_ADDR 0x7F

#define PI_MAX_I2C_DEVICE_COUNT (1<<16)

/* max pi_i2c_msg_t per transaction */

#define  PI_I2C_RDRW_IOCTL_MAX_MSGS 42

/* flags for i2cTransaction, pi_i2c_msg_t */

#define PI_I2C_M_WR           0x0000 /* write data */
#define PI_I2C_M_RD           0x0001 /* read data */
#define PI_I2C_M_TEN          0x0010 /* ten bit chip address */
#define PI_I2C_M_RECV_LEN     0x0400 /* length will be first received byte */
#define PI_I2C_M_NO_RD_ACK    0x0800 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define PI_I2C_M_IGNORE_NAK   0x1000 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define PI_I2C_M_REV_DIR_ADDR 0x2000 /* if I2C_FUNC_PROTOCOL_MANGLING */
#define PI_I2C_M_NOSTART      0x4000 /* if I2C_FUNC_PROTOCOL_MANGLING */

/* bbI2CZip and i2cZip commands */

#define PI_I2C_END          0
#define PI_I2C_ESC          1
#define PI_I2C_START        2
#define PI_I2C_COMBINED_ON  2
#define PI_I2C_STOP         3
#define PI_I2C_COMBINED_OFF 3
#define PI_I2C_ADDR         4
#define PI_I2C_FLAGS        5
#define PI_I2C_READ         6
#define PI_I2C_WRITE        7

typedef struct
{
   uint16_t addr;  /* slave address       */
   uint16_t flags;
   uint16_t len;   /* msg length          */
   uint8_t  *buf;  /* pointer to msg data */
} pi_i2c_msg_t;

/*F*/
int i2cOpen(unsigned i2cBus, unsigned i2cAddr, unsigned i2cFlags);
/*D
This returns a handle for the device at the address on the I2C bus.

. .
  i2cBus: 0-1
 i2cAddr: 0x00-0x7F
i2cFlags: 0
. .

No flags are currently defined.  This parameter should be set to zero.

Returns a handle (>=0) if OK, otherwise PI_BAD_I2C_BUS, PI_BAD_I2C_ADDR,
PI_BAD_FLAGS, PI_NO_HANDLE, or PI_I2C_OPEN_FAILED.

For the SMBus commands the low level transactions are shown at the end
of the function description.  The following abbreviations are used.

. .
S      (1 bit) : Start bit
P      (1 bit) : Stop bit
Rd/Wr  (1 bit) : Read/Write bit. Rd equals 1, Wr equals 0.
A, NA  (1 bit) : Accept and not accept bit.
Addr   (7 bits): I2C 7 bit address.
i2cReg (8 bits): Command byte, a byte which often selects a register.
Data   (8 bits): A data byte.
Count  (8 bits): A byte defining the length of a block operation.

[..]: Data sent by the device.
. .
D*/


/*F*/
int i2cClose(unsigned handle);
/*D
This closes the I2C device associated with the handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE.
D*/


/*F*/
int i2cWriteQuick(unsigned handle, unsigned bit);
/*D
This sends a single bit (in the Rd/Wr bit) to the device associated
with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
   bit: 0-1, the value to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

Quick command. SMBus 2.0 5.5.1
. .
S Addr bit [A] P
. .
D*/


/*F*/
int i2cWriteByte(unsigned handle, unsigned bVal);
/*D
This sends a single byte to the device associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
  bVal: 0-0xFF, the value to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

Send byte. SMBus 2.0 5.5.2
. .
S Addr Wr [A] bVal [A] P
. .
D*/


/*F*/
int i2cReadByte(unsigned handle);
/*D
This reads a single byte from the device associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
. .

Returns the byte read (>=0) if OK, otherwise PI_BAD_HANDLE,
or PI_I2C_READ_FAILED.

Receive byte. SMBus 2.0 5.5.3
. .
S Addr Rd [A] [Data] NA P
. .
D*/


/*F*/
int i2cWriteByteData(unsigned handle, unsigned i2cReg, unsigned bVal);
/*D
This writes a single byte to the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write
  bVal: 0-0xFF, the value to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

Write byte. SMBus 2.0 5.5.4
. .
S Addr Wr [A] i2cReg [A] bVal [A] P
. .
D*/


/*F*/
int i2cWriteWordData(unsigned handle, unsigned i2cReg, unsigned wVal);
/*D
This writes a single 16 bit word to the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write
  wVal: 0-0xFFFF, the value to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

Write word. SMBus 2.0 5.5.4
. .
S Addr Wr [A] i2cReg [A] wValLow [A] wValHigh [A] P
. .
D*/


/*F*/
int i2cReadByteData(unsigned handle, unsigned i2cReg);
/*D
This reads a single byte from the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to read
. .

Returns the byte read (>=0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

Read byte. SMBus 2.0 5.5.5
. .
S Addr Wr [A] i2cReg [A] S Addr Rd [A] [Data] NA P
. .
D*/


/*F*/
int i2cReadWordData(unsigned handle, unsigned i2cReg);
/*D
This reads a single 16 bit word from the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to read
. .

Returns the word read (>=0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

Read word. SMBus 2.0 5.5.5
. .
S Addr Wr [A] i2cReg [A] S Addr Rd [A] [DataLow] A [DataHigh] NA P
. .
D*/


/*F*/
int i2cProcessCall(unsigned handle, unsigned i2cReg, unsigned wVal);
/*D
This writes 16 bits of data to the specified register of the device
associated with handle and reads 16 bits of data in return.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write/read
  wVal: 0-0xFFFF, the value to write
. .

Returns the word read (>=0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

Process call. SMBus 2.0 5.5.6
. .
S Addr Wr [A] i2cReg [A] wValLow [A] wValHigh [A]
   S Addr Rd [A] [DataLow] A [DataHigh] NA P
. .
D*/


/*F*/
int i2cWriteBlockData(
unsigned handle, unsigned i2cReg, char *buf, unsigned count);
/*D
This writes up to 32 bytes to the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write
   buf: an array with the data to send
 count: 1-32, the number of bytes to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

Block write. SMBus 2.0 5.5.7
. .
S Addr Wr [A] i2cReg [A] count [A]
   buf0 [A] buf1 [A] ... [A] bufn [A] P
. .
D*/


/*F*/
int i2cReadBlockData(unsigned handle, unsigned i2cReg, char *buf);
/*D
This reads a block of up to 32 bytes from the specified register of
the device associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to read
   buf: an array to receive the read data
. .

The amount of returned data is set by the device.

Returns the number of bytes read (>=0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

Block read. SMBus 2.0 5.5.7
. .
S Addr Wr [A] i2cReg [A]
   S Addr Rd [A] [Count] A [buf0] A [buf1] A ... A [bufn] NA P
. .
D*/


/*F*/
int i2cBlockProcessCall(
unsigned handle, unsigned i2cReg, char *buf, unsigned count);
/*D
This writes data bytes to the specified register of the device
associated with handle and reads a device specified number
of bytes of data in return.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write/read
   buf: an array with the data to send and to receive the read data
 count: 1-32, the number of bytes to write
. .

Returns the number of bytes read (>=0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

The SMBus 2.0 documentation states that a minimum of 1 byte may be
sent and a minimum of 1 byte may be received.  The total number of
bytes sent/received must be 32 or less.

Block write-block read. SMBus 2.0 5.5.8
. .
S Addr Wr [A] i2cReg [A] count [A] buf0 [A] ... bufn [A]
   S Addr Rd [A] [Count] A [buf0] A ... [bufn] A P
. .
D*/


/*F*/
int i2cReadI2CBlockData(
unsigned handle, unsigned i2cReg, char *buf, unsigned count);
/*D
This reads count bytes from the specified register of the device
associated with handle .  The count may be 1-32.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to read
   buf: an array to receive the read data
 count: 1-32, the number of bytes to read
. .

Returns the number of bytes read (>0) if OK, otherwise PI_BAD_HANDLE,
PI_BAD_PARAM, or PI_I2C_READ_FAILED.

. .
S Addr Wr [A] i2cReg [A]
   S Addr Rd [A] [buf0] A [buf1] A ... A [bufn] NA P
. .
D*/


/*F*/
int i2cWriteI2CBlockData(
unsigned handle, unsigned i2cReg, char *buf, unsigned count);
/*D
This writes 1 to 32 bytes to the specified register of the device
associated with handle.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
i2cReg: 0-255, the register to write
   buf: the data to write
 count: 1-32, the number of bytes to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

. .
S Addr Wr [A] i2cReg [A] buf0 [A] buf1 [A] ... [A] bufn [A] P
. .
D*/

/*F*/
int i2cReadDevice(unsigned handle, char *buf, unsigned count);
/*D
This reads count bytes from the raw device into buf.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
   buf: an array to receive the read data bytes
 count: >0, the number of bytes to read
. .

Returns count (>0) if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_READ_FAILED.

. .
S Addr Rd [A] [buf0] A [buf1] A ... A [bufn] NA P
. .
D*/


/*F*/
int i2cWriteDevice(unsigned handle, char *buf, unsigned count);
/*D
This writes count bytes from buf to the raw device.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
   buf: an array containing the data bytes to write
 count: >0, the number of bytes to write
. .

Returns 0 if OK, otherwise PI_BAD_HANDLE, PI_BAD_PARAM, or
PI_I2C_WRITE_FAILED.

. .
S Addr Wr [A] buf0 [A] buf1 [A] ... [A] bufn [A] P
. .
D*/

/*F*/
void i2cSwitchCombined(int setting);
/*D
This sets the I2C (i2c-bcm2708) module "use combined transactions"
parameter on or off.

. .
setting: 0 to set the parameter off, non-zero to set it on
. .


NOTE: when the flag is on a write followed by a read to the same
slave address will use a repeated start (rather than a stop/start).
D*/

/*F*/
int i2cSegments(unsigned handle, pi_i2c_msg_t *segs, unsigned numSegs);
/*D
This function executes multiple I2C segments in one transaction by
calling the I2C_RDWR ioctl.

. .
 handle: >=0, as returned by a call to [*i2cOpen*]
   segs: an array of I2C segments
numSegs: >0, the number of I2C segments
. .

Returns the number of segments if OK, otherwise PI_BAD_I2C_SEG.
D*/

/*F*/
int i2cZip(
   unsigned handle,
   char    *inBuf,
   unsigned inLen,
   char    *outBuf,
   unsigned outLen);
/*D
This function executes a sequence of I2C operations.  The
operations to be performed are specified by the contents of inBuf
which contains the concatenated command codes and associated data.

. .
handle: >=0, as returned by a call to [*i2cOpen*]
 inBuf: pointer to the concatenated I2C commands, see below
 inLen: size of command buffer
outBuf: pointer to buffer to hold returned data
outLen: size of output buffer
. .

Returns >= 0 if OK (the number of bytes read), otherwise
PI_BAD_HANDLE, PI_BAD_POINTER, PI_BAD_I2C_CMD, PI_BAD_I2C_RLEN.
PI_BAD_I2C_WLEN, or PI_BAD_I2C_SEG.

The following command codes are supported:

Name    @ Cmd & Data @ Meaning
End     @ 0          @ No more commands
Escape  @ 1          @ Next P is two bytes
On      @ 2          @ Switch combined flag on
Off     @ 3          @ Switch combined flag off
Address @ 4 P        @ Set I2C address to P
Flags   @ 5 lsb msb  @ Set I2C flags to lsb + (msb << 8)
Read    @ 6 P        @ Read P bytes of data
Write   @ 7 P ...    @ Write P bytes of data

The address, read, and write commands take a parameter P.
Normally P is one byte (0-255).  If the command is preceded by
the Escape command then P is two bytes (0-65535, least significant
byte first).

The address defaults to that associated with the handle.
The flags default to 0.  The address and flags maintain their
previous value until updated.

The returned I2C data is stored in consecutive locations of outBuf.

...
Set address 0x53, write 0x32, read 6 bytes
Set address 0x1E, write 0x03, read 6 bytes
Set address 0x68, write 0x1B, read 8 bytes
End

0x04 0x53   0x07 0x01 0x32   0x06 0x06
0x04 0x1E   0x07 0x01 0x03   0x06 0x06
0x04 0x68   0x07 0x01 0x1B   0x06 0x08
0x00
...
D*/

/*F*/
int bbI2COpen(unsigned SDA, unsigned SCL, unsigned baud);
/*D
This function selects a pair of gpios for bit banging I2C at a
specified baud rate.

Bit banging I2C allows for certain operations which are not possible
with the standard I2C driver.

o baud rates as low as 50
o repeated starts
o clock stretching
o I2C on any pair of spare gpios

. .
 SDA: 0-31
 SCL: 0-31
baud: 50-500000
. .

Returns 0 if OK, otherwise PI_BAD_USER_GPIO, PI_BAD_I2C_BAUD, or
PI_GPIO_IN_USE.

NOTE:

The gpios used for SDA and SCL must have pull-ups to 3V3 connected.  As
a guide the hardware pull-ups on pins 3 and 5 are 1k8 in value.
D*/

/*F*/
int bbI2CClose(unsigned SDA);
/*D
This function stops bit banging I2C on a pair of gpios previously
opened with [*bbI2COpen*].

. .
SDA: 0-31, the SDA gpio used in a prior call to [*bbI2COpen*]
. .

Returns 0 if OK, otherwise PI_BAD_USER_GPIO, or PI_NOT_I2C_GPIO.
D*/

/*F*/
int bbI2CZip(
   unsigned SDA,
   char    *inBuf,
   unsigned inLen,
   char    *outBuf,
   unsigned outLen);
/*D
This function executes a sequence of bit banged I2C operations.  The
operations to be performed are specified by the contents of inBuf
which contains the concatenated command codes and associated data.

. .
   SDA: 0-31 (as used in a prior call to [*bbI2COpen*])
 inBuf: pointer to the concatenated I2C commands, see below
 inLen: size of command buffer
outBuf: pointer to buffer to hold returned data
outLen: size of output buffer
. .

Returns >= 0 if OK (the number of bytes read), otherwise
PI_BAD_USER_GPIO, PI_NOT_I2C_GPIO, PI_BAD_POINTER,
PI_BAD_I2C_CMD, PI_BAD_I2C_RLEN, PI_BAD_I2C_WLEN,
PI_I2C_READ_FAILED, or PI_I2C_WRITE_FAILED.

The following command codes are supported:

Name    @ Cmd & Data   @ Meaning
End     @ 0            @ No more commands
Escape  @ 1            @ Next P is two bytes
Start   @ 2            @ Start condition
Stop    @ 3            @ Stop condition
Address @ 4 P          @ Set I2C address to P
Flags   @ 5 lsb msb    @ Set I2C flags to lsb + (msb << 8)
Read    @ 6 P          @ Read P bytes of data
Write   @ 7 P ...      @ Write P bytes of data

The address, read, and write commands take a parameter P.
Normally P is one byte (0-255).  If the command is preceded by
the Escape command then P is two bytes (0-65535, least significant
byte first).

The address and flags default to 0.  The address and flags maintain
their previous value until updated.

No flags are currently defined.

The returned I2C data is stored in consecutive locations of outBuf.

...
Set address 0x53
start, write 0x32, (re)start, read 6 bytes, stop
Set address 0x1E
start, write 0x03, (re)start, read 6 bytes, stop
Set address 0x68
start, write 0x1B, (re)start, read 8 bytes, stop
End

0x04 0x53
0x02 0x07 0x01 0x32   0x02 0x06 0x06 0x03

0x04 0x1E
0x02 0x07 0x01 0x03   0x02 0x06 0x06 0x03

0x04 0x68
0x02 0x07 0x01 0x1B   0x02 0x06 0x08 0x03

0x00
...
D*/

#endif // I2C_H

