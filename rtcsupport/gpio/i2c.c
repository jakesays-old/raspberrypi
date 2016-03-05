/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

/* pigpio version 46 */

/* include ------------------------------------------------------- */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <syslog.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "i2c.h"
#include "private.h"

typedef struct
{
	uint16_t state;
	int16_t  fd;
	uint32_t addr;
	uint32_t flags;
	uint32_t funcs;
} i2cInfo_t;

static i2cInfo_t        i2cInfo[PI_I2C_SLOTS];

#define PI_I2C_RETRIES 0x0701
#define PI_I2C_TIMEOUT 0x0702
#define PI_I2C_SLAVE   0x0703
#define PI_I2C_FUNCS   0x0705
#define PI_I2C_RDWR    0x0707
#define PI_I2C_SMBUS   0x0720

#define PI_I2C_SMBUS_READ  1
#define PI_I2C_SMBUS_WRITE 0

#define PI_I2C_SMBUS_QUICK            0
#define PI_I2C_SMBUS_BYTE             1
#define PI_I2C_SMBUS_BYTE_DATA        2
#define PI_I2C_SMBUS_WORD_DATA        3
#define PI_I2C_SMBUS_PROC_CALL        4
#define PI_I2C_SMBUS_BLOCK_DATA       5
#define PI_I2C_SMBUS_I2C_BLOCK_BROKEN 6
#define PI_I2C_SMBUS_BLOCK_PROC_CALL  7
#define PI_I2C_SMBUS_I2C_BLOCK_DATA   8

#define PI_I2C_SMBUS_BLOCK_MAX     32
#define PI_I2C_SMBUS_I2C_BLOCK_MAX 32

#define PI_I2C_FUNC_SMBUS_QUICK            0x00010000
#define PI_I2C_FUNC_SMBUS_READ_BYTE        0x00020000
#define PI_I2C_FUNC_SMBUS_WRITE_BYTE       0x00040000
#define PI_I2C_FUNC_SMBUS_READ_BYTE_DATA   0x00080000
#define PI_I2C_FUNC_SMBUS_WRITE_BYTE_DATA  0x00100000
#define PI_I2C_FUNC_SMBUS_READ_WORD_DATA   0x00200000
#define PI_I2C_FUNC_SMBUS_WRITE_WORD_DATA  0x00400000
#define PI_I2C_FUNC_SMBUS_PROC_CALL        0x00800000
#define PI_I2C_FUNC_SMBUS_READ_BLOCK_DATA  0x01000000
#define PI_I2C_FUNC_SMBUS_WRITE_BLOCK_DATA 0x02000000
#define PI_I2C_FUNC_SMBUS_READ_I2C_BLOCK   0x04000000
#define PI_I2C_FUNC_SMBUS_WRITE_I2C_BLOCK  0x08000000

union my_smbus_data
{
	uint8_t  byte;
	uint16_t word;
	uint8_t  block[PI_I2C_SMBUS_BLOCK_MAX + 2];
};

struct my_smbus_ioctl_data
{
	uint8_t read_write;
	uint8_t command;
	uint32_t size;
	union my_smbus_data *data;
};

typedef struct
{
	pi_i2c_msg_t *msgs; /* pointers to pi_i2c_msgs */
	uint32_t     nmsgs; /* number of pi_i2c_msgs */
} my_i2c_rdwr_ioctl_data_t;

static int myI2CGetPar(char *inBuf, int *inPos, int inLen, int *esc);
static int my_smbus_access(
   int fd,
	char rw,
	uint8_t cmd,
	int size,
	union my_smbus_data *data);

static int read_SDA(wfRx_t *w)
{
	myGpioSetMode(w->I.SDA, PI_INPUT);
	return gpioRead(w->I.SDA);
}

static void set_SDA(wfRx_t *w)
{
	myGpioSetMode(w->I.SDA, PI_INPUT);
}

static void clear_SDA(wfRx_t *w)
{
	myGpioSetMode(w->I.SDA, PI_OUTPUT);
	myGpioWrite(w->I.SDA, 0);
}

static void clear_SCL(wfRx_t *w)
{
	myGpioSetMode(w->I.SCL, PI_OUTPUT);
	myGpioWrite(w->I.SCL, 0);
}

static void I2C_delay(wfRx_t *w)
{
	myGpioDelay(w->I.delay);
}

static void I2C_clock_stretch(wfRx_t *w)
{
	uint32_t now, max_stretch = 10000;

	myGpioSetMode(w->I.SCL, PI_INPUT);
	now = gpioTick();
	while ((gpioRead(w->I.SCL) == 0) && ((gpioTick() - now) < max_stretch))
		;
}

static void I2CStart(wfRx_t *w)
{
	if (w->I.started)
	{
		set_SDA(w);
		I2C_delay(w);
		I2C_clock_stretch(w);
		I2C_delay(w);
	}

	clear_SDA(w);
	I2C_delay(w);
	clear_SCL(w);
	I2C_delay(w);

	w->I.started = 1;
}

static void I2CStop(wfRx_t *w)
{
	clear_SDA(w);
	I2C_delay(w);
	I2C_clock_stretch(w);
	I2C_delay(w);
	set_SDA(w);
	I2C_delay(w);

	w->I.started = 0;
}

static void I2CPutBit(wfRx_t *w, int bit)
{
	if (bit)
		set_SDA(w);
	else
		clear_SDA(w);

	I2C_delay(w);
	I2C_clock_stretch(w);
	I2C_delay(w);
	clear_SCL(w);
}

static int I2CGetBit(wfRx_t *w)
{
	int bit;

	set_SDA(w); /* let SDA float */
	I2C_delay(w);
	I2C_clock_stretch(w);
	bit = read_SDA(w);
	I2C_delay(w);
	clear_SCL(w);

	return bit;
}

static int I2CPutByte(wfRx_t *w, int byte)
{
	int bit, nack;

	for (bit = 0; bit < 8; bit++)
	{
		I2CPutBit(w, byte & 0x80);
		byte <<= 1;
	}

	nack = I2CGetBit(w);

	return nack;
}

static uint8_t I2CGetByte(wfRx_t *w, int nack)
{
	int bit, byte = 0;

	for (bit = 0; bit < 8; bit++)
	{
		byte = (byte << 1) | I2CGetBit(w);
	}

	I2CPutBit(w, nack);

	return byte;
}

int bbI2COpen(unsigned SDA, unsigned SCL, unsigned baud)
{
	DBG(DBG_USER, "SDA=%d SCL=%d baud=%d", SDA, SCL, baud);

	CHECK_INITED;

	if (SDA > PI_MAX_USER_GPIO)
		SOFT_ERROR(PI_BAD_USER_GPIO, "bad SDA (%d)", SDA);

	if (SCL > PI_MAX_USER_GPIO)
		SOFT_ERROR(PI_BAD_USER_GPIO, "bad SCL (%d)", SCL);

	if ((baud < PI_BB_I2C_MIN_BAUD) || (baud > PI_BB_I2C_MAX_BAUD))
		SOFT_ERROR(PI_BAD_I2C_BAUD,
			"SDA %d, bad baud rate (%d)",
			SDA,
			baud);

	if (wfRx[SDA].mode != PI_WFRX_NONE)
		SOFT_ERROR(PI_GPIO_IN_USE, "gpio %d is already being used", SDA);

	if ((wfRx[SCL].mode != PI_WFRX_NONE)  || (SCL == SDA))
		SOFT_ERROR(PI_GPIO_IN_USE, "gpio %d is already being used", SCL);

	wfRx[SDA].gpio = SDA;
	wfRx[SDA].mode = PI_WFRX_I2C;
	wfRx[SDA].baud = baud;

	wfRx[SDA].I.started = 0;
	wfRx[SDA].I.SDA = SDA;
	wfRx[SDA].I.SCL = SCL;
	wfRx[SDA].I.delay = 500000 / baud;
	wfRx[SDA].I.SDAMode = gpioGetMode(SDA);
	wfRx[SDA].I.SCLMode = gpioGetMode(SCL);

	wfRx[SCL].gpio = SCL;
	wfRx[SCL].mode = PI_WFRX_I2C_CLK;

	myGpioSetMode(SDA, PI_INPUT);
	myGpioSetMode(SCL, PI_INPUT);

	return 0;
}


/* ----------------------------------------------------------------------- */


int bbI2CClose(unsigned SDA)
{
	DBG(DBG_USER, "SDA=%d", SDA);

	CHECK_INITED;

	if (SDA > PI_MAX_USER_GPIO)
		SOFT_ERROR(PI_BAD_USER_GPIO, "bad gpio (%d)", SDA);

	switch (wfRx[SDA].mode)
	{
	case PI_WFRX_I2C:

		gpioSetMode(wfRx[SDA].I.SDA, wfRx[SDA].I.SDAMode);
		gpioSetMode(wfRx[SDA].I.SCL, wfRx[SDA].I.SCLMode);

		wfRx[wfRx[SDA].I.SDA].mode = PI_WFRX_NONE;
		wfRx[wfRx[SDA].I.SCL].mode = PI_WFRX_NONE;

		break;

	default:

		SOFT_ERROR(PI_NOT_I2C_GPIO, "no I2C on gpio (%d)", SDA);

		break;

	}

	return 0;
}

/*-------------------------------------------------------------------------*/

int bbI2CZip(
   unsigned SDA,
	char *inBuf,
	unsigned inLen,
	char *outBuf,
	unsigned outLen)
{
	int i, ack, inPos, outPos, status, bytes;
	int addr, flags, esc, setesc;
	wfRx_t *w;

	DBG(DBG_USER,
		"gpio=%d inBuf=%s outBuf=%08X len=%d",
		SDA,
		myBuf2Str(inLen, (char *)inBuf),
		(int)outBuf,
		outLen);

	CHECK_INITED;

	if (SDA > PI_MAX_USER_GPIO)
		SOFT_ERROR(PI_BAD_USER_GPIO, "bad gpio (%d)", SDA);

	if (wfRx[SDA].mode != PI_WFRX_I2C)
		SOFT_ERROR(PI_NOT_I2C_GPIO, "no I2C on gpio (%d)", SDA);

	if (!inBuf || !inLen)
		SOFT_ERROR(PI_BAD_POINTER, "input buffer can't be NULL");

	if (!outBuf && outLen)
		SOFT_ERROR(PI_BAD_POINTER, "output buffer can't be NULL");

	w = &wfRx[SDA];

	inPos = 0;
	outPos = 0;
	status = 0;

	addr = 0;
	flags = 0;
	esc = 0;
	setesc = 0;

	while (!status && (inPos < inLen))
	{
		DBG(DBG_INTERNAL,
			"status=%d inpos=%d inlen=%d cmd=%d addr=%d flags=%x",
			status,
			inPos,
			inLen,
			inBuf[inPos],
			addr,
			flags);

		switch (inBuf[inPos++])
		{
		case PI_I2C_END:
			status = 1;
			break;

		case PI_I2C_START:
			I2CStart(w);
			break;

		case PI_I2C_STOP:
			I2CStop(w);
			break;

		case PI_I2C_ADDR:
			addr = myI2CGetPar(inBuf, &inPos, inLen, &esc);
			if (addr < 0) status = PI_BAD_I2C_CMD;
			break;

		case PI_I2C_FLAGS:
		   /* cheat to force two byte flags */
			esc = 1;
			flags = myI2CGetPar(inBuf, &inPos, inLen, &esc);
			if (flags < 0) status = PI_BAD_I2C_CMD;
			break;

		case PI_I2C_ESC:
			setesc = 1;
			break;

		case PI_I2C_READ:

			bytes = myI2CGetPar(inBuf, &inPos, inLen, &esc);

			if (bytes >= 0)
				ack = I2CPutByte(w, (addr << 1) | 1);

			if (bytes > 0)
			{
				if (!ack)
				{
					if ((bytes + outPos) < outLen)
					{
						for (i = 0; i < (bytes - 1); i++)
						{
							outBuf[outPos++] = I2CGetByte(w, 0);
						}
						outBuf[outPos++] = I2CGetByte(w, 1);
					}
					else 
						status = PI_BAD_I2C_RLEN;
				}
				else
					status = PI_I2C_READ_FAILED;
			}
			else
				status = PI_BAD_I2C_CMD;
			break;

		case PI_I2C_WRITE:

			bytes = myI2CGetPar(inBuf, &inPos, inLen, &esc);

			if (bytes >= 0)
				ack = I2CPutByte(w, addr << 1);

			if (bytes > 0)
			{
				if (!ack)
				{
					if ((bytes + inPos) < inLen)
					{
						for (i = 0; i < (bytes - 1); i++)
						{
							ack = I2CPutByte(w, inBuf[inPos++]);
							if (ack) status = PI_I2C_WRITE_FAILED;
						}
						ack = I2CPutByte(w, inBuf[inPos++]);
					}
					else 
						status = PI_BAD_I2C_RLEN;
				}
				else
					status = PI_I2C_WRITE_FAILED;
			}
			else
				status = PI_BAD_I2C_CMD;
			break;

		default:
			status = PI_BAD_I2C_CMD;
		}

		if (setesc) 
			esc = 1;
		else 
			esc = 0;

		setesc = 0;
	}

	if (status >= 0) 
		status = outPos;

	return status;
}


int i2cWriteQuick(unsigned handle, unsigned bit)
{
	int status;

	DBG(DBG_USER, "handle=%d bit=%d", handle, bit);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_QUICK) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (bit > 1)
		SOFT_ERROR(PI_BAD_PARAM, "bad bit (%d)", bit);

	status = my_smbus_access(
		i2cInfo[handle].fd,
		bit,
		0,
		PI_I2C_SMBUS_QUICK,
		NULL);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}

int i2cReadByte(unsigned handle)
{
	union my_smbus_data data;
	int status;

	DBG(DBG_USER, "handle=%d", handle);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_READ_BYTE) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_READ,
		0,
		PI_I2C_SMBUS_BYTE,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}

	return 0xFF & data.byte;
}


int i2cWriteByte(unsigned handle, unsigned bVal)
{
	int status;

	DBG(DBG_USER, "handle=%d bVal=%d", handle, bVal);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_WRITE_BYTE) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (bVal > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad bVal (%d)", bVal);

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		bVal,
		PI_I2C_SMBUS_BYTE,
		NULL);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}


int i2cReadByteData(unsigned handle, unsigned reg)
{
	union my_smbus_data data;
	int status;

	DBG(DBG_USER, "handle=%d reg=%d", handle, reg);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_READ_BYTE_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	status = my_smbus_access(i2cInfo[handle].fd,
		PI_I2C_SMBUS_READ,
		reg,
		PI_I2C_SMBUS_BYTE_DATA,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}

	return 0xFF & data.byte;
}


int i2cWriteByteData(unsigned handle, unsigned reg, unsigned bVal)
{
	union my_smbus_data data;

	int status;

	DBG(DBG_USER, "handle=%d reg=%d bVal=%d", handle, reg, bVal);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_WRITE_BYTE_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if (bVal > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad bVal (%d)", bVal);

	data.byte = bVal;

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_BYTE_DATA,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}


int i2cReadWordData(unsigned handle, unsigned reg)
{
	union my_smbus_data data;
	int status;

	DBG(DBG_USER, "handle=%d reg=%d", handle, reg);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_READ_WORD_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	status = (my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_READ,
		reg,
		PI_I2C_SMBUS_WORD_DATA,
		&data));

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}

	return 0xFFFF & data.word;
}


int i2cWriteWordData(unsigned handle, unsigned reg, unsigned wVal)
{
	union my_smbus_data data;

	int status;

	DBG(DBG_USER, "handle=%d reg=%d wVal=%d", handle, reg, wVal);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_WRITE_WORD_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if (wVal > 0xFFFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad wVal (%d)", wVal);

	data.word = wVal;

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_WORD_DATA,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}


int i2cProcessCall(unsigned handle, unsigned reg, unsigned wVal)
{
	union my_smbus_data data;
	int status;

	DBG(DBG_USER, "handle=%d reg=%d wVal=%d", handle, reg, wVal);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_PROC_CALL) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if (wVal > 0xFFFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad wVal (%d)", wVal);

	data.word = wVal;

	status = (my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_PROC_CALL,
		&data));

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}

	return 0xFFFF & data.word;
}


int i2cReadBlockData(unsigned handle, unsigned reg, char* buf)
{
	union my_smbus_data data;

	int i, status;

	DBG(DBG_USER, "handle=%d reg=%d buf=%08X", handle, reg, (unsigned)buf);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_READ_BLOCK_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	status = (my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_READ,
		reg,
		PI_I2C_SMBUS_BLOCK_DATA,
		&data));

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}
	else
	{
		if (data.block[0] <= PI_I2C_SMBUS_BLOCK_MAX)
		{
			for (i = 0; i < data.block[0]; i++)
				buf[i] = data.block[i + 1];
			return data.block[0];
		}
		else
			return PI_I2C_READ_FAILED ;
	}
}


int i2cWriteBlockData(
	unsigned handle,
	unsigned reg,
	char* buf,
	unsigned count)
{
	union my_smbus_data data;

	int i, status;

	DBG(DBG_USER,
		"handle=%d reg=%d count=%d [%s]",
		handle,
		reg,
		count,
		myBuf2Str(count, buf));

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_WRITE_BLOCK_DATA) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if ((count < 1) || (count > 32))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	for (i = 1; i <= count; i++)
		data.block[i] = buf[i - 1];
	data.block[0] = count;

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_BLOCK_DATA,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}


int i2cBlockProcessCall(
	unsigned handle,
	unsigned reg,
	char* buf,
	unsigned count)
{
	union my_smbus_data data;

	int i, status;

	DBG(DBG_USER,
		"handle=%d reg=%d count=%d [%s]",
		handle,
		reg,
		count,
		myBuf2Str(count, buf));

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_PROC_CALL) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if ((count < 1) || (count > 32))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	for (i = 1; i <= count; i++)
		data.block[i] = buf[i - 1];
	data.block[0] = count;

	status = (my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_BLOCK_PROC_CALL,
		&data));

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}
	else
	{
		if (data.block[0] <= PI_I2C_SMBUS_BLOCK_MAX)
		{
			for (i = 0; i < data.block[0]; i++)
				buf[i] = data.block[i + 1];
			return data.block[0];
		}
		else
			return PI_I2C_READ_FAILED ;
	}
}


int i2cReadI2CBlockData(
	unsigned handle,
	unsigned reg,
	char* buf,
	unsigned count)
{
	union my_smbus_data data;

	int i, status;
	uint32_t size;

	DBG(DBG_USER,
		"handle=%d reg=%d count=%d buf=%08X",
		handle,
		reg,
		count,
		(unsigned)buf);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_READ_I2C_BLOCK) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if ((count < 1) || (count > 32))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	if (count == 32)
		size = PI_I2C_SMBUS_I2C_BLOCK_BROKEN;
	else
		size = PI_I2C_SMBUS_I2C_BLOCK_DATA;

	data.block[0] = count;

	status = (my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_READ,
		reg,
		size,
		&data));

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_READ_FAILED ;
	}
	else
	{
		if (data.block[0] <= PI_I2C_SMBUS_I2C_BLOCK_MAX)
		{
			for (i = 0; i < data.block[0]; i++)
				buf[i] = data.block[i + 1];
			return data.block[0];
		}
		else
			return PI_I2C_READ_FAILED ;
	}
}


int i2cWriteI2CBlockData(
	unsigned handle,
	unsigned reg,
	char* buf,
	unsigned count)
{
	union my_smbus_data data;

	int i, status;

	DBG(DBG_USER,
		"handle=%d reg=%d count=%d [%s]",
		handle,
		reg,
		count,
		myBuf2Str(count, buf));

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((i2cInfo[handle].funcs & PI_I2C_FUNC_SMBUS_WRITE_I2C_BLOCK) == 0)
		SOFT_ERROR(PI_BAD_SMBUS_CMD, "SMBUS command not supported by driver");

	if (reg > 0xFF)
		SOFT_ERROR(PI_BAD_PARAM, "bad reg (%d)", reg);

	if ((count < 1) || (count > 32))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	for (i = 1; i <= count; i++)
		data.block[i] = buf[i - 1];

	data.block[0] = count;

	status = my_smbus_access(
		i2cInfo[handle].fd,
		PI_I2C_SMBUS_WRITE,
		reg,
		PI_I2C_SMBUS_I2C_BLOCK_BROKEN,
		&data);

	if (status < 0)
	{
		DBG(DBG_USER, "error=%d (%m)", status);
		return PI_I2C_WRITE_FAILED ;
	}

	return status;
}

int i2cWriteDevice(unsigned handle, char* buf, unsigned count)
{
	int bytes;

	DBG(DBG_USER,
		"handle=%d count=%d [%s]",
		handle,
		count,
		myBuf2Str(count, buf));

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((count < 1) || (count > PI_MAX_I2C_DEVICE_COUNT))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	bytes = write(i2cInfo[handle].fd, buf, count);

	if (bytes != count)
	{
		DBG(DBG_USER, "error=%d (%m)", bytes);
		return PI_I2C_WRITE_FAILED ;
	}

	return 0;
}

int i2cReadDevice(unsigned handle, char* buf, unsigned count)
{
	int bytes;

	DBG(DBG_USER,
		"handle=%d count=%d buf=%08X",
		handle,
		count,
		(unsigned)buf);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state == PI_I2C_CLOSED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if ((count < 1) || (count > PI_MAX_I2C_DEVICE_COUNT))
		SOFT_ERROR(PI_BAD_PARAM, "bad count (%d)", count);

	bytes = read(i2cInfo[handle].fd, buf, count);

	if (bytes != count)
	{
		DBG(DBG_USER, "error=%d (%m)", bytes);
		return PI_I2C_READ_FAILED ;
	}

	return bytes;
}

int i2cOpen(unsigned i2cBus, unsigned i2cAddr, unsigned i2cFlags)
{
	char dev[32];
	int i, slot, fd;
	uint32_t funcs;

	DBG(DBG_USER,
		"i2cBus=%d i2cAddr=%d flags=0x%X",
		i2cBus,
		i2cAddr,
		i2cFlags);

	CHECK_INITED;

	if (i2cBus >= PI_NUM_I2C_BUS)
		SOFT_ERROR(PI_BAD_I2C_BUS, "bad I2C bus (%d)", i2cBus);

	if (i2cAddr > PI_MAX_I2C_ADDR)
		SOFT_ERROR(PI_BAD_I2C_ADDR, "bad I2C address (%d)", i2cAddr);

	if (i2cFlags)
		SOFT_ERROR(PI_BAD_FLAGS, "bad flags (0x%X)", i2cFlags);

	slot = -1;

	for (i = 0; i < PI_I2C_SLOTS; i++)
	{
		if (i2cInfo[i].state == PI_I2C_CLOSED)
		{
			i2cInfo[i].state = PI_I2C_OPENED;
			slot = i;
			break;
		}
	}

	if (slot < 0)
		SOFT_ERROR(PI_NO_HANDLE, "no I2C handles");

	sprintf(dev, "/dev/i2c-%d", i2cBus);

	if ((fd = open(dev, O_RDWR)) < 0)
	{
		/* try a modprobe */

		system("/sbin/modprobe i2c_dev");
		system("/sbin/modprobe i2c_bcm2708");

		myGpioDelay(100000);

		if ((fd = open(dev, O_RDWR)) < 0)
		{
			i2cInfo[slot].state = PI_I2C_CLOSED;
			return PI_I2C_OPEN_FAILED ;
		}
	}

	if (ioctl(fd, PI_I2C_SLAVE, i2cAddr) < 0)
	{
		close(fd);
		i2cInfo[slot].state = PI_I2C_CLOSED;
		return PI_I2C_OPEN_FAILED ;
	}

	if (ioctl(fd, PI_I2C_FUNCS, &funcs) < 0)
	{
		funcs = -1; /* assume all smbus commands allowed */
	}

	i2cInfo[slot].fd = fd;
	i2cInfo[slot].addr = i2cAddr;
	i2cInfo[slot].flags = i2cFlags;
	i2cInfo[slot].funcs = funcs;

	return slot;
}

int i2cClose(unsigned handle)
{
	DBG(DBG_USER, "handle=%d", handle);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state != PI_I2C_OPENED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].fd >= 0)
		close(i2cInfo[handle].fd);

	i2cInfo[handle].fd = -1;
	i2cInfo[handle].state = PI_I2C_CLOSED;

	return 0;
}

void i2cSwitchCombined(int setting)
{
	int fd;

	DBG(DBG_USER, "setting=%d", setting);

	fd = open(PI_I2C_COMBINED, O_WRONLY);

	if (fd >= 0)
	{
		if (setting)
			write(fd, "1\n", 2);
		else
			write(fd, "0\n", 2);

		close(fd);
	}
}

int i2cSegments(unsigned handle, pi_i2c_msg_t* segs, unsigned numSegs)
{
	int retval;
	my_i2c_rdwr_ioctl_data_t rdwr;

	DBG(DBG_USER, "handle=%d", handle);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state != PI_I2C_OPENED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (segs == NULL)
		SOFT_ERROR(PI_BAD_POINTER, "null segments");

	if (numSegs > PI_I2C_RDRW_IOCTL_MAX_MSGS)
		SOFT_ERROR(PI_TOO_MANY_SEGS, "too many segments (%d)", numSegs);

	rdwr.msgs = segs;
	rdwr.nmsgs = numSegs;

	retval = ioctl(i2cInfo[handle].fd, PI_I2C_RDWR, &rdwr);

	if (retval >= 0)
	{
		return retval;
	}
	int err = errno;
	printf("ioctl error: (%d) %s\n", err, strerror(err));
	return PI_BAD_I2C_SEG ;
}

int i2cZip(
	unsigned handle,
	char* inBuf,
	unsigned inLen,
	char* outBuf,
	unsigned outLen)
{
	int numSegs, inPos, outPos, status, bytes, flags, addr;
	int esc, setesc;
	pi_i2c_msg_t segs[PI_I2C_RDRW_IOCTL_MAX_MSGS];

	DBG(DBG_USER,
		"handle=%d inBuf=%s outBuf=%08X len=%d",
		handle,
		myBuf2Str(inLen, (char *)inBuf),
		(int)outBuf,
		outLen);

	CHECK_INITED;

	if (handle >= PI_I2C_SLOTS)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (i2cInfo[handle].state != PI_I2C_OPENED)
		SOFT_ERROR(PI_BAD_HANDLE, "bad handle (%d)", handle);

	if (!inBuf || !inLen)
		SOFT_ERROR(PI_BAD_POINTER, "input buffer can't be NULL");

	if (!outBuf && outLen)
		SOFT_ERROR(PI_BAD_POINTER, "output buffer can't be NULL");

	numSegs = 0;

	inPos = 0;
	outPos = 0;
	status = 0;

	addr = i2cInfo[handle].addr;
	flags = 0;
	esc = 0;
	setesc = 0;

	while (!status && (inPos < inLen))
	{
		DBG(DBG_INTERNAL,
			"status=%d inpos=%d inlen=%d cmd=%d addr=%d flags=%x",
			status,
			inPos,
			inLen,
			inBuf[inPos],
			addr,
			flags);

		switch (inBuf[inPos++])
		{
		case PI_I2C_END:
			status = 1;
			break;

		case PI_I2C_COMBINED_ON:
			/* Run prior transactions before setting combined flag */
			if (numSegs)
			{
				status = i2cSegments(handle, segs, numSegs);
				if (status >= 0)
					status = 0; /* continue */
				numSegs = 0;
			}
			i2cSwitchCombined(1);
			break;

		case PI_I2C_COMBINED_OFF:
			/* Run prior transactions before clearing combined flag */
			if (numSegs)
			{
				status = i2cSegments(handle, segs, numSegs);
				if (status >= 0)
					status = 0; /* continue */
				numSegs = 0;
			}
			i2cSwitchCombined(0);
			break;

		case PI_I2C_ADDR:
			addr = myI2CGetPar(inBuf, &inPos, inLen, &esc);
			if (addr < 0)
				status = PI_BAD_I2C_CMD ;
			break;

		case PI_I2C_FLAGS:
			/* cheat to force two byte flags */
			esc = 1;
			flags = myI2CGetPar(inBuf, &inPos, inLen, &esc);
			if (flags < 0)
				status = PI_BAD_I2C_CMD ;
			break;

		case PI_I2C_ESC:
			setesc = 1;
			break;

		case PI_I2C_READ:

			bytes = myI2CGetPar(inBuf, &inPos, inLen, &esc);

			if (bytes >= 0)
			{
				if ((bytes + outPos) < outLen)
				{
					segs[numSegs].addr = addr;
					segs[numSegs].flags = (flags | 1);
					segs[numSegs].len = bytes;
					segs[numSegs].buf = (uint8_t *)(outBuf + outPos);
					outPos += bytes;
					numSegs++;
					if (numSegs >= PI_I2C_RDRW_IOCTL_MAX_MSGS)
					{
						status = i2cSegments(handle, segs, numSegs);
						if (status >= 0)
							status = 0; /* continue */
						numSegs = 0;
					}
				}
				else
					status = PI_BAD_I2C_RLEN ;
			}
			else
				status = PI_BAD_I2C_RLEN ;
			break;

		case PI_I2C_WRITE:

			bytes = myI2CGetPar(inBuf, &inPos, inLen, &esc);

			if (bytes >= 0)
			{
				if ((bytes + inPos) < inLen)
				{
					segs[numSegs].addr = addr;
					segs[numSegs].flags = (flags & 0xfffe);
					segs[numSegs].len = bytes;
					segs[numSegs].buf = (uint8_t *)(inBuf + inPos);
					inPos += bytes;
					numSegs++;
					if (numSegs >= PI_I2C_RDRW_IOCTL_MAX_MSGS)
					{
						status = i2cSegments(handle, segs, numSegs);
						if (status >= 0)
							status = 0; /* continue */
						numSegs = 0;
					}
				}
				else
					status = PI_BAD_I2C_WLEN ;
			}
			else
				status = PI_BAD_I2C_WLEN ;
			break;

		default:
			status = PI_BAD_I2C_CMD ;
		}

		if (setesc)
			esc = 1;
		else
			esc = 0;

		setesc = 0;
	}

	if (status >= 0)
	{
		if (numSegs)
			status = i2cSegments(handle, segs, numSegs);
	}

	if (status >= 0)
		status = outPos;

	return status;
}

static int myI2CGetPar(char *inBuf, int *inPos, int inLen, int *esc)
{
	int bytes;

	if (*esc) bytes = 2;
	else bytes = 1;

	*esc = 0;

	if (*inPos <= (inLen - bytes))
	{
		if (bytes == 1)
		{
			return inBuf[(*inPos)++];
		}
		else
		{
			(*inPos) += 2;
			return inBuf[*inPos - 2] + (inBuf[*inPos - 1] << 8);
		}
	}
	return -1;
}

static int my_smbus_access(
   int fd,
	char rw,
	uint8_t cmd,
	int size,
	union my_smbus_data *data)
{
	struct my_smbus_ioctl_data args;

	DBG(DBG_INTERNAL,
		"rw=%d reg=%d cmd=%d data=%s",
		rw,
		cmd,
		size,
		myBuf2Str(data->byte + 1, (char*)data));

	args.read_write = rw;
	args.command    = cmd;
	args.size       = size;
	args.data       = data;

	return ioctl(fd, PI_I2C_SMBUS, &args);
}
