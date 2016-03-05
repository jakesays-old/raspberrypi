
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

#include "pigpio.h"
#include "pierrors.h"
#include "i2c.h"
#include "private.h"
#include "command.h"

/* resources which must be released on gpioTerminate */

FILE* inpFifo = NULL;
FILE* outFifo = NULL;
int fdSock       = -1;
int pthFifoRunning   = 0;
int pthSocketRunning = 0;
pthread_t pthFifo;
pthread_t pthSocket;


int myDoCommand(uint32_t *p, unsigned bufSize, char *buf)
{
   int res, i, j;
   uint32_t mask;
   uint32_t tmp1, tmp2, tmp3;
   gpioPulse_t *pulse;
   int masked;

   res = 0;

   switch (p[0])
   {
      case PI_CMD_BC1:
         mask = gpioMask;

         res = gpioWrite_Bits_0_31_Clear(p[1]&mask);

         if ((mask | p[1]) != mask)
         {
            DBG(DBG_USER,
               "gpioWrite_Bits_0_31_Clear: bad bits %08X (permissions %08X)",
               p[1], mask);
            res = PI_SOME_PERMITTED;
         }
         break;

      case PI_CMD_BC2:
         mask = gpioMask>>32;

         res = gpioWrite_Bits_32_53_Clear(p[1]&mask);

         if ((mask | p[1]) != mask)
         {
            DBG(DBG_USER,
               "gpioWrite_Bits_32_53_Clear: bad bits %08X (permissions %08X)",
               p[1], mask);
            res = PI_SOME_PERMITTED;
         }
         break;

      case PI_CMD_BI2CC: res = bbI2CClose(p[1]); break;

      case PI_CMD_BI2CO:
         memcpy(&p[4], buf, 4);
         res = bbI2COpen(p[1], p[2], p[4]);
         break;

      case PI_CMD_BI2CZ:
         /* use half buffer for write, half buffer for read */
         if (p[3] > (bufSize/2)) p[3] = bufSize/2;
         res = bbI2CZip(p[1], buf, p[3], buf+(bufSize/2), bufSize/2);
         if (res > 0)
         {
            memcpy(buf, buf+(bufSize/2), res);
         }
         break;

      case PI_CMD_BR1: res = gpioRead_Bits_0_31(); break;

      case PI_CMD_BR2: res = gpioRead_Bits_32_53(); break;

      case PI_CMD_BS1:
         mask = gpioMask;

         res = gpioWrite_Bits_0_31_Set(p[1]&mask);

         if ((mask | p[1]) != mask)
         {
            DBG(DBG_USER,
               "gpioWrite_Bits_0_31_Set: bad bits %08X (permissions %08X)",
               p[1], mask);
            res = PI_SOME_PERMITTED;
         }
         break;

      case PI_CMD_BS2:
         mask = gpioMask>>32;

         res = gpioWrite_Bits_32_53_Set(p[1]&mask);

         if ((mask | p[1]) != mask)
         {
            DBG(DBG_USER,
               "gpioWrite_Bits_32_53_Set: bad bits %08X (permissions %08X)",
               p[1], mask);
            res = PI_SOME_PERMITTED;
         }
         break;

      case PI_CMD_CF1:
         res = gpioCustom1(p[1], p[2], buf, p[3]);
         break;

      case PI_CMD_CF2:
         /* a couple of extra precautions for untrusted code */
         if (p[2] > bufSize) p[2] = bufSize;
         res = gpioCustom2(p[1], buf, p[3], buf, p[2]);
         if (res > p[2]) res = p[2];
         break;

      case PI_CMD_CGI: res = gpioCfgGetInternals(); break;

      case PI_CMD_CSI: res = gpioCfgSetInternals(p[1]); break;

      case PI_CMD_FG:
         res = gpioGlitchFilter(p[1], p[2]);
         break;

      case PI_CMD_FN:
         memcpy(&p[4], buf, 4);
         res = gpioNoiseFilter(p[1], p[2], p[4]);
         break;

      case PI_CMD_GDC: res = gpioGetPWMdutycycle(p[1]); break;

      case PI_CMD_GPW: res = gpioGetServoPulsewidth(p[1]); break;

      case PI_CMD_HC:
         /* special case to allow password in upper byte */
         if (myPermit(p[1]&0xFFFFFF)) res = gpioHardwareClock(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioHardwareClock: gpio %d, no permission to update",
                p[1] & 0xFFFFFF);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_HELP: break;

      case PI_CMD_HP:
         if (myPermit(p[1]))
         {
            memcpy(&p[4], buf, 4);
            res = gpioHardwarePWM(p[1], p[2], p[4]);
         }
         else
         {
            DBG(DBG_USER,
               "gpioHardwarePWM: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_HWVER:
           res = gpioHardwareRevision(); break;



      case PI_CMD_I2CC:
           res = i2cClose(p[1]); break;

      case PI_CMD_I2CO:
         memcpy(&p[4], buf, 4);
         res = i2cOpen(p[1], p[2], p[4]);
         break;

      case PI_CMD_I2CPC:
         memcpy(&p[4], buf, 4);
         res = i2cProcessCall(p[1], p[2], p[4]);
         break;

      case PI_CMD_I2CPK:
         res = i2cBlockProcessCall(p[1], p[2], buf, p[3]);
         break;

      case PI_CMD_I2CRB:
           res = i2cReadByteData(p[1], p[2]);
           break;

      case PI_CMD_I2CRD:
         if (p[2] > bufSize) p[2] = bufSize;
         res = i2cReadDevice(p[1], buf, p[2]);
         break;

      case PI_CMD_I2CRI:
         memcpy(&p[4], buf, 4);
         res = i2cReadI2CBlockData(p[1], p[2], buf, p[4]);
         break;

      case PI_CMD_I2CRK:
         res = i2cReadBlockData(p[1], p[2], buf);
         break;

      case PI_CMD_I2CRS:
           res = i2cReadByte(p[1]);
           break;

      case PI_CMD_I2CRW:
           res = i2cReadWordData(p[1], p[2]);
           break;

      case PI_CMD_I2CWB:
         memcpy(&p[4], buf, 4);
         res = i2cWriteByteData(p[1], p[2], p[4]);
         break;

      case PI_CMD_I2CWD:
         res = i2cWriteDevice(p[1], buf, p[3]);
         break;

      case PI_CMD_I2CWI:
         res = i2cWriteI2CBlockData(p[1], p[2], buf, p[3]);
         break;

      case PI_CMD_I2CWK:
         res = i2cWriteBlockData(p[1], p[2], buf, p[3]);
         break;

      case PI_CMD_I2CWQ:
           res = i2cWriteQuick(p[1], p[2]);
           break;

      case PI_CMD_I2CWS:
           res = i2cWriteByte(p[1], p[2]);
           break;

      case PI_CMD_I2CWW:
         memcpy(&p[4], buf, 4);
         res = i2cWriteWordData(p[1], p[2], p[4]);
         break;

      case PI_CMD_I2CZ:
         /* use half buffer for write, half buffer for read */
         if (p[3] > (bufSize/2))
             p[3] = bufSize/2;
         res = i2cZip(p[1], buf, p[3], buf+(bufSize/2), bufSize/2);
         if (res > 0)
         {
            memcpy(buf, buf+(bufSize/2), res);
         }
         break;



      case PI_CMD_MICS:
         if (p[1] <= PI_MAX_MICS_DELAY)
             myGpioDelay(p[1]);
         else res = PI_BAD_MICS_DELAY;
         break;

      case PI_CMD_MILS:
         if (p[1] <= PI_MAX_MILS_DELAY)
             myGpioDelay(p[1] * 1000);
         else res = PI_BAD_MILS_DELAY;
         break;

      case PI_CMD_MODEG: res = gpioGetMode(p[1]); break;

      case PI_CMD_MODES:
         if (myPermit(p[1]))
             res = gpioSetMode(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioSetMode: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_NB:
           res = gpioNotifyBegin(p[1], p[2]);
           break;

      case PI_CMD_NC:
           res = gpioNotifyClose(p[1]);
           break;

      case PI_CMD_NO:
           res = gpioNotifyOpen();
           break;

      case PI_CMD_NP:
           res = gpioNotifyPause(p[1]);
           break;

      case PI_CMD_PFG:
           res = gpioGetPWMfrequency(p[1]);
           break;

      case PI_CMD_PFS:
         if (myPermit(p[1]))
             res = gpioSetPWMfrequency(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioSetPWMfrequency: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_PIGPV:
           res = gpioVersion();
           break;

      case PI_CMD_PRG: res = gpioGetPWMrange(p[1]); break;

      case PI_CMD_PROC:
         res = gpioStoreScript(buf);
         break;

      case PI_CMD_PROCD: res = gpioDeleteScript(p[1]); break;

      case PI_CMD_PROCP:
         res = gpioScriptStatus(p[1], (uint32_t *)buf);
         break;

      case PI_CMD_PROCR:
         res = gpioRunScript(p[1], p[3]/4, (uint32_t *)buf);
         break;

      case PI_CMD_PROCS: res = gpioStopScript(p[1]); break;

      case PI_CMD_PRRG: res = gpioGetPWMrealRange(p[1]); break;

      case PI_CMD_PRS:
         if (myPermit(p[1])) res = gpioSetPWMrange(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioSetPWMrange: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_PUD:
         if (myPermit(p[1])) res = gpioSetPullUpDown(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioSetPullUpDown: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_PWM:
         if (myPermit(p[1])) res = gpioPWM(p[1], p[2]);
         else
         {
            DBG(DBG_USER, "gpioPWM: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_READ: res = gpioRead(p[1]); break;

      case PI_CMD_SERVO:
         if (myPermit(p[1])) res = gpioServo(p[1], p[2]);
         else
         {
            DBG(DBG_USER,
               "gpioServo: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;



      case PI_CMD_SERRB: res = serReadByte(p[1]); break;

      case PI_CMD_SERWB: res = serWriteByte(p[1], p[2]); break;

      case PI_CMD_SERC: res = serClose(p[1]); break;

      case PI_CMD_SERDA: res = serDataAvailable(p[1]); break;

      case PI_CMD_SERO: res = serOpen(buf, p[1], p[2]); break;

      case PI_CMD_SERR:
         if (p[2] > bufSize) p[2] = bufSize;
         res = serRead(p[1], buf, p[2]);
         break;

      case PI_CMD_SERW: res = serWrite(p[1], buf, p[3]); break;



      case PI_CMD_SLR:
         if (p[2] > bufSize) p[2] = bufSize;
         res = gpioSerialRead(p[1], buf, p[2]);
         break;

      case PI_CMD_SLRC: res = gpioSerialReadClose(p[1]); break;

      case PI_CMD_SLRO:
            memcpy(&p[4], buf, 4);
            res = gpioSerialReadOpen(p[1], p[2], p[4]); break;

      case PI_CMD_SLRI: res = gpioSerialReadInvert(p[1], p[2]); break;


      case PI_CMD_SPIC: res = spiClose(p[1]); break;

      case PI_CMD_SPIO:
         memcpy(&p[4], buf, 4);
         res = spiOpen(p[1], p[2], p[4]);
         break;

      case PI_CMD_SPIR:
         if (p[2] > bufSize) p[2] = bufSize;
         res = spiRead(p[1], buf, p[2]);
         break;

      case PI_CMD_SPIW:
         if (p[3] > bufSize) p[3] = bufSize;
         res = spiWrite(p[1], buf, p[3]);
         break;

      case PI_CMD_SPIX:
         if (p[3] > bufSize) p[3] = bufSize;
         res = spiXfer(p[1], buf, buf, p[3]);
         break;

      case PI_CMD_TICK: res = gpioTick(); break;

      case PI_CMD_TRIG:
         if (myPermit(p[1]))
         {
            memcpy(&p[4], buf, 4);
            res = gpioTrigger(p[1], p[2], p[4]);
         }
         else
         {
            DBG(DBG_USER,
               "gpioTrigger: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_WDOG: res = gpioSetWatchdog(p[1], p[2]); break;

      case PI_CMD_WRITE:
         if (myPermit(p[1])) res = gpioWrite(p[1], p[2]);
         else
         {
            DBG(DBG_USER, "gpioWrite: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;



      case PI_CMD_WVAG:

         /* need to mask off any non permitted gpios */

         mask = gpioMask;
         pulse = (gpioPulse_t *)buf;
         j = p[3]/sizeof(gpioPulse_t);
         masked = 0;

         for (i=0; i<j; i++)
         {
            tmp1 = pulse[i].gpioOn & mask;
            if (tmp1 != pulse[i].gpioOn)
            {
               pulse[i].gpioOn = tmp1;
               masked = 1;
            }

            tmp1 = pulse[i].gpioOff & mask;
            if (tmp1 != pulse[i].gpioOff)
            {
               pulse[i].gpioOff = tmp1;
               masked = 1;
            }
            DBG(DBG_SCRIPT, "on=%X off=%X delay=%d",
               pulse[i].gpioOn, pulse[i].gpioOff, pulse[i].usDelay);
         }

         res = gpioWaveAddGeneric(j, pulse);

         /* report permission error unless another error occurred */
         if (masked && (res >= 0)) res = PI_SOME_PERMITTED;

         break;

      case PI_CMD_WVAS:
         if (myPermit(p[1]))
         {
            memcpy(&tmp1, buf, 4);   /* databits */
            memcpy(&tmp2, buf+4, 4); /* stophalfbits */
            memcpy(&tmp3, buf+8, 4); /* offset */
            res = gpioWaveAddSerial
               (p[1], p[2], tmp1, tmp2, tmp3, p[3]-12, buf+12);
         }
         else
         {
            DBG(
               DBG_USER,
               "gpioWaveAddSerial: gpio %d, no permission to update", p[1]);
            res = PI_NOT_PERMITTED;
         }
         break;

      case PI_CMD_WVBSY: res = gpioWaveTxBusy(); break;

      case PI_CMD_WVCHA:
         if (p[3] > bufSize) p[3] = bufSize;
         res = gpioWaveChain(buf, p[3]);
         break;


      case PI_CMD_WVCLR: res = gpioWaveClear(); break;

      case PI_CMD_WVCRE: res = gpioWaveCreate(); break;

      case PI_CMD_WVDEL: res = gpioWaveDelete(p[1]); break;

      case PI_CMD_WVGO:  res = gpioWaveTxStart(PI_WAVE_MODE_ONE_SHOT); break;

      case PI_CMD_WVGOR: res = gpioWaveTxStart(PI_WAVE_MODE_REPEAT); break;

      case PI_CMD_WVHLT: res = gpioWaveTxStop(); break;

      case PI_CMD_WVNEW: res = gpioWaveAddNew(); break;

      case PI_CMD_WVSC:
         switch(p[1])
         {
            case 0: res = gpioWaveGetCbs();     break;
            case 1: res = gpioWaveGetHighCbs(); break;
            case 2: res = gpioWaveGetMaxCbs();  break;
            default: res = PI_BAD_WVSC_COMMND;
         }
         break;

      case PI_CMD_WVSM:
         switch(p[1])
         {
            case 0: res = gpioWaveGetMicros();     break;
            case 1: res = gpioWaveGetHighMicros(); break;
            case 2: res = gpioWaveGetMaxMicros();  break;
            default: res = PI_BAD_WVSM_COMMND;
         }
         break;

      case PI_CMD_WVSP:
         switch(p[1])
         {
            case 0: res = gpioWaveGetPulses();     break;
            case 1: res = gpioWaveGetHighPulses(); break;
            case 2: res = gpioWaveGetMaxPulses();  break;
            default: res = PI_BAD_WVSP_COMMND;
         }
         break;

      case PI_CMD_WVTX:
         res = gpioWaveTxSend(p[1], PI_WAVE_MODE_ONE_SHOT); break;

      case PI_CMD_WVTXM:
         res = gpioWaveTxSend(p[1], p[2]); break;

      case PI_CMD_WVTXR:
         res = gpioWaveTxSend(p[1], PI_WAVE_MODE_REPEAT); break;

      default:
         res = PI_UNKNOWN_COMMAND;
         break;
   }

   return res;
}

static void myCreatePipe(char * name, int perm)
{
   unlink(name);

   mkfifo(name, perm);

   if (chmod(name, perm) < 0)
   {
      DBG(DBG_ALWAYS, "Can't set permissions (%d) for %s, %m", perm, name);
      return;
   }
}

static void * pthFifoThread(void *x)
{
   char buf[CMD_MAX_EXTENSION];
   int idx, flags, len, res, i;
   uint32_t p[CMD_P_ARR];
   cmdCtlParse_t ctl;
   uint32_t *param;
   char v[CMD_MAX_EXTENSION];

   myCreatePipe(PI_INPFIFO, 0662);

   if ((inpFifo = fopen(PI_INPFIFO, "r+")) == NULL)
      SOFT_ERROR((void*)PI_INIT_FAILED, "fopen %s failed(%m)", PI_INPFIFO);

   myCreatePipe(PI_OUTFIFO, 0664);

   if ((outFifo = fopen(PI_OUTFIFO, "w+")) == NULL)
      SOFT_ERROR((void*)PI_INIT_FAILED, "fopen %s failed (%m)", PI_OUTFIFO);

   /* set outFifo non-blocking */

   flags = fcntl(fileno(outFifo), F_GETFL, 0);
   fcntl(fileno(outFifo), F_SETFL, flags | O_NONBLOCK);

   /* don't start until DMA started */

   spinWhileStarting();

   while (1)
   {
      if (fgets(buf, sizeof(buf), inpFifo) == NULL)
         SOFT_ERROR((void*)PI_INIT_FAILED, "fifo fgets failed (%m)");

      len = strlen(buf);

      if (len)
      {
        --len;
        buf[len] = 0; /* replace terminating */
      }

      ctl.eaten = 0;
      idx = 0;

      while (((ctl.eaten)<len) && (idx >= 0))
      {
         if ((idx=cmdParse(buf, p, CMD_MAX_EXTENSION, v, &ctl)) >= 0)
         {
            /* make sure extensions are null terminated */

            v[p[3]] = 0;

            res = myDoCommand(p, sizeof(v)-1, v);

            switch (cmdInfo[idx].rv)
            {
               case 0:
                  fprintf(outFifo, "%d\n", res);
                  break;

               case 1:
                  fprintf(outFifo, "%d\n", res);
                  break;

               case 2:
                  fprintf(outFifo, "%d\n", res);
                  break;

               case 3:
                  fprintf(outFifo, "%08X\n", res);
                  break;

               case 4:
                  fprintf(outFifo, "%u\n", res);
                  break;

               case 5:
                  fprintf(outFifo, cmdUsage);
                  break;

               case 6:
                  fprintf(outFifo, "%d", res);
                  if (res > 0)
                  {
                     for (i=0; i<res; i++)
                     {
                        fprintf(outFifo, " %d", v[i]);
                     }
                  }
                  fprintf(outFifo, "\n");
                  break;

               case 7:
                  if (res < 0) fprintf(outFifo, "%d\n", res);
                  else
                  {
                     fprintf(outFifo, "%d", res);
                     param = (uint32_t *)v;
                     for (i=0; i<PI_MAX_SCRIPT_PARAMS; i++)
                     {
                        fprintf(outFifo, " %d", param[i]);
                     }
                     fprintf(outFifo, "\n");
                  }
                  break;
            }
         }
         else fprintf(outFifo, "%d\n", PI_BAD_FIFO_COMMAND);
      }

      fflush(outFifo);
   }

   return 0;
}

static void closeOrphanedNotifications(int slot, int fd)
{
   int i;

   /* Check for and close any orphaned notifications. */

   for (i=0; i<PI_NOTIFY_SLOTS; i++)
   {
      if ((i != slot) &&
          (gpioNotify[i].state != PI_NOTIFY_CLOSED) &&
          (gpioNotify[i].fd == fd))
      {
         DBG(DBG_USER, "closed orphaned fd=%d (handle=%d)", fd, i);
         gpioNotify[i].state = PI_NOTIFY_CLOSED;
         intNotifyBits();
      }
   }
}

static int gpioNotifyOpenInBand(int fd)
{
   int i, slot;

   DBG(DBG_USER, "fd=%d", fd);

   CHECK_INITED;

   slot = -1;

   for (i=0; i<PI_NOTIFY_SLOTS; i++)
   {
      if (gpioNotify[i].state == PI_NOTIFY_CLOSED)
      {
         slot = i;
         break;
      }
   }

   if (slot < 0) SOFT_ERROR(PI_NO_HANDLE, "no handle");

   gpioNotify[slot].state = PI_NOTIFY_OPENED;
   gpioNotify[slot].seqno = 0;
   gpioNotify[slot].bits  = 0;
   gpioNotify[slot].fd    = fd;
   gpioNotify[slot].pipe  = 0;
   gpioNotify[slot].max_emits  = MAX_EMITS;
   gpioNotify[slot].lastReportTick = gpioTick();

   closeOrphanedNotifications(slot, fd);

   return slot;
}

int gpioNotifyOpenWithSize(int bufSize)
{
   int i, slot, fd;
   char name[32];

   DBG(DBG_USER, "bufSize=%d", bufSize);

   CHECK_INITED;

   slot = -1;

   for (i=0; i<PI_NOTIFY_SLOTS; i++)
   {
      if (gpioNotify[i].state == PI_NOTIFY_CLOSED)
      {
         gpioNotify[i].state = PI_NOTIFY_OPENED;
         slot = i;
         break;
      }
   }

   if (slot < 0)
      SOFT_ERROR(PI_NO_HANDLE, "no handle");

   sprintf(name, "/dev/pigpio%d", slot);

   myCreatePipe(name, 0664);

   fd = open(name, O_RDWR|O_NONBLOCK);

   if (fd < 0)
   {
      gpioNotify[slot].state = PI_NOTIFY_CLOSED;
      SOFT_ERROR(PI_BAD_PATHNAME, "open %s failed (%m)", name);
   }

   if (bufSize != 0)
   {
      i = fcntl(fd, F_SETPIPE_SZ, bufSize);
      if (i != bufSize)
      {
         gpioNotify[slot].state = PI_NOTIFY_CLOSED;
         SOFT_ERROR(PI_BAD_PATHNAME,
            "fcntl %s size %d failed (%m)", name, bufSize);
      }
   }

   gpioNotify[slot].seqno = 0;
   gpioNotify[slot].bits  = 0;
   gpioNotify[slot].fd    = fd;
   gpioNotify[slot].pipe  = 1;
   gpioNotify[slot].max_emits  = MAX_EMITS;
   gpioNotify[slot].lastReportTick = gpioTick();

   closeOrphanedNotifications(slot, fd);

   return slot;
}

int gpioNotifyOpen(void)
{
   return gpioNotifyOpenWithSize(0);
}

static void *pthSocketThreadHandler(void *fdC)
{
   int sock = *(int*)fdC;
   uint32_t p[10];
   int opt;
   char buf[CMD_MAX_EXTENSION];

   free(fdC);

   /* Disable the Nagle algorithm. */
   opt = 1;
   setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(int));

   while (1)
   {
      if (recv(sock, p, 16, MSG_WAITALL) != 16) break;

      if (p[3])
      {
         if (p[3] < sizeof(buf))
         {
            /* read extension into buf */
            if (recv(sock, buf, p[3], MSG_WAITALL) != p[3])
            {
               /* Serious error.  No point continuing. */
               DBG(DBG_ALWAYS,
                  "recv failed for %d bytes, sock=%d", p[3], sock);

               closeOrphanedNotifications(-1, sock);

               close(sock);

               return 0;
            }
         }
         else
         {
            /* Serious error.  No point continuing. */
            DBG(DBG_ALWAYS,
               "ext too large %d(%d), sock=%d", p[3], sizeof(buf), sock);

            closeOrphanedNotifications(-1, sock);

            close(sock);

            return 0;
         }
      }

      /* add null terminator in case it's a string */

      buf[p[3]] = 0;

      switch (p[0])
      {
         case PI_CMD_NOIB:

            p[3] = gpioNotifyOpenInBand(sock);

           /* Enable the Nagle algorithm. */
            opt = 0;
            setsockopt(
               sock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(int));

            break;

         case PI_CMD_PROCP:
            p[3] = myDoCommand(p, sizeof(buf)-1, buf+sizeof(int));
            if (((int)p[3]) >= 0)
            {
               memcpy(buf, &p[3], 4);
               p[3] = 4 + (4*PI_MAX_SCRIPT_PARAMS);
            }
            break;

         default:
            p[3] = myDoCommand(p, sizeof(buf)-1, buf);
      }

      write(sock, p, 16);

      switch (p[0])
      {
         /* extensions */

         case PI_CMD_BI2CZ:
         case PI_CMD_CF2:
         case PI_CMD_I2CPK:
         case PI_CMD_I2CRD:
         case PI_CMD_I2CRI:
         case PI_CMD_I2CRK:
         case PI_CMD_I2CZ:
         case PI_CMD_PROCP:
         case PI_CMD_SERR:
         case PI_CMD_SLR:
         case PI_CMD_SPIX:
         case PI_CMD_SPIR:

            if (((int)p[3]) > 0)
            {
               write(sock, buf, p[3]);
            }
            break;

         default:
           break;
      }
   }

   closeOrphanedNotifications(-1, sock);

   close(sock);

   return 0;
}

/* ----------------------------------------------------------------------- */

static void * pthSocketThread(void *x)
{
   int fdC, c, *sock;
   struct sockaddr_in client;
   pthread_attr_t attr;

   if (pthread_attr_init(&attr))
      SOFT_ERROR((void*)PI_INIT_FAILED,
         "pthread_attr_init failed (%m)");

   if (pthread_attr_setstacksize(&attr, STACK_SIZE))
      SOFT_ERROR((void*)PI_INIT_FAILED,
         "pthread_attr_setstacksize failed (%m)");

   if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED))
      SOFT_ERROR((void*)PI_INIT_FAILED,
         "pthread_attr_setdetachstate failed (%m)");

   /* fdSock opened in gpioInitialise so that we can treat
      failure to bind as fatal. */

   listen(fdSock, 100);

   c = sizeof(struct sockaddr_in);

   /* don't start until DMA started */

   spinWhileStarting();

   while ((fdC =
      accept(fdSock, (struct sockaddr *)&client, (socklen_t*)&c)))
   {
      pthread_t thr;

      closeOrphanedNotifications(-1, fdC);

      sock = malloc(sizeof(int));

      *sock = fdC;

      if (pthread_create
         (&thr, &attr, pthSocketThreadHandler, (void*) sock) < 0)
         SOFT_ERROR((void*)PI_INIT_FAILED,
            "socket pthread_create failed (%m)");
   }

   if (fdC < 0)
      SOFT_ERROR((void*)PI_INIT_FAILED, "accept failed (%m)");

   return 0;
}

int initRemote(pthread_attr_t* pthAttr)
{
    struct sockaddr_in server;
    char * portStr;
    unsigned port;
    int i;

    if (!(gpioCfg.ifFlags & PI_DISABLE_FIFO_IF))
    {
       if (pthread_create(&pthFifo, pthAttr, pthFifoThread, &i))
          SOFT_ERROR(PI_INIT_FAILED, "pthread_create fifo failed (%m)");

       pthFifoRunning = 1;
    }

    if (!(gpioCfg.ifFlags & PI_DISABLE_SOCK_IF))
    {
       fdSock = socket(AF_INET , SOCK_STREAM , 0);

       if (fdSock == -1)
          SOFT_ERROR(PI_INIT_FAILED, "socket failed (%m)");

       portStr = getenv(PI_ENVPORT);

       if (portStr)
           port = atoi(portStr);
       else
           port = gpioCfg.socketPort;

       server.sin_family = AF_INET;
       if (gpioCfg.ifFlags & PI_LOCALHOST_SOCK_IF)
       {
          server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
       }
       else
       {
          server.sin_addr.s_addr = htonl(INADDR_ANY);
       }
       server.sin_port = htons(port);

       if (bind(fdSock,(struct sockaddr *)&server , sizeof(server)) < 0)
          SOFT_ERROR(PI_INIT_FAILED, "bind to port %d failed (%m)", port);

       if (pthread_create(&pthSocket, pthAttr, pthSocketThread, &i))
          SOFT_ERROR(PI_INIT_FAILED, "pthread_create socket failed (%m)");

       pthSocketRunning = 1;
    }

    return 0;
}
