#ifndef PRIVATE_H
#define PRIVATE_H

#include "pigpio.h"
#include "command.h"

#ifndef EMBEDDED_IN_VM
#define DBG(level, format, arg...) DO_DBG(level, format, ## arg)
#else
#define DBG(level, format, arg...)
#endif

#define DO_DBG(level, format, arg...)                              \
   {                                                               \
      if (gpioCfg.dbgLevel >= level)                               \
         fprintf(stderr, "%s %s: " format "\n" ,                   \
            myTimeStamp(), __FUNCTION__ , ## arg);                 \
   }

#define DBG_MIN_LEVEL 0
#define DBG_ALWAYS    0
#define DBG_STARTUP   1
#define DBG_DMACBS    2
#define DBG_SCRIPT    3
#define DBG_USER      4
#define DBG_INTERNAL  5
#define DBG_SLOW_TICK 6
#define DBG_FAST_TICK 7
#define DBG_MAX_LEVEL 8

extern int libInitialised;

#define STACK_SIZE (256*1024)


#ifndef DISABLE_SER_CHECK_INITED
#define SER_CHECK_INITED CHECK_INITED
#else
#define SER_CHECK_INITED
#endif

#define CHECK_INITED                                               \
   do                                                              \
   {                                                               \
      if (!libInitialised)                                         \
      {                                                            \
         DBG(DBG_ALWAYS,                                           \
           "pigpio uninitialised, call gpioInitialise()");         \
         return PI_NOT_INITIALISED;                                \
      }                                                            \
   }                                                               \
   while (0)

#define CHECK_INITED_RET_NULL_PTR                                  \
   do                                                              \
   {                                                               \
      if (!libInitialised)                                         \
      {                                                            \
         DBG(DBG_ALWAYS,                                           \
           "pigpio uninitialised, call gpioInitialise()");         \
         return (NULL);                                            \
      }                                                            \
   }                                                               \
   while (0)

#define CHECK_INITED_RET_NIL                                       \
   do                                                              \
   {                                                               \
      if (!libInitialised)                                         \
      {                                                            \
         DBG(DBG_ALWAYS,                                           \
           "pigpio uninitialised, call gpioInitialise()");         \
      }                                                            \
   }                                                               \
   while (0)

#define CHECK_NOT_INITED                                           \
   do                                                              \
   {                                                               \
      if (libInitialised)                                          \
      {                                                            \
         DBG(DBG_ALWAYS,                                           \
            "pigpio initialised, call gpioTerminate()");           \
         return PI_INITIALISED;                                    \
      }                                                            \
   }                                                               \
   while (0)

#define SOFT_ERROR(x, format, arg...)                              \
   do                                                              \
   {                                                               \
      DBG(DBG_ALWAYS, format, ## arg);                             \
      return x;                                                    \
   }                                                               \
   while (0)


typedef struct
{
	unsigned bufferMilliseconds;
	unsigned clockMicros;
	unsigned clockPeriph;
	unsigned DMAprimaryChannel;
	unsigned DMAsecondaryChannel;
	unsigned socketPort;
	unsigned ifFlags;
	unsigned memAllocMode;
	unsigned dbgLevel;
	unsigned alertFreq;
	uint32_t internals;
	   /*
	   0-3: dbgLevel
	   4-7: alertFreq
	   */
} gpioCfg_t;

#define PI_I2C_CLOSED 0
#define PI_I2C_OPENED 1

extern char * myTimeStamp();

typedef struct
{
	char    *buf;
	uint32_t bufSize;
	int      readPos;
	int      writePos;
	uint32_t fullBit; /* nanoseconds */
	uint32_t halfBit; /* nanoseconds */
	int      timeout; /* millisconds */
	uint32_t startBitTick; /* microseconds */
	uint32_t nextBitDiff; /* nanoseconds */
	int      bit;
	uint32_t data;
	int      bytes; /* 1, 2, 4 */
	int      level;
	int      dataBits; /* 1-32 */
	int      invert; /* 0, 1 */
} wfRxSerial_t;

typedef struct
{
	int SDA;
	int SCL;
	int delay;
	int SDAMode;
	int SCLMode;
	int started;
} wfRxI2C_t;

typedef struct
{
	int      mode;
	int      gpio;
	uint32_t baud;
	union
	{
		wfRxSerial_t s;
		wfRxI2C_t    I;
	};
} wfRx_t;

extern wfRx_t wfRx[PI_MAX_USER_GPIO + 1];
extern volatile gpioCfg_t gpioCfg;

#define PI_WFRX_NONE    0
#define PI_WFRX_SERIAL  1
#define PI_WFRX_I2C     2
#define PI_WFRX_I2C_CLK 3

extern void myGpioSetMode(unsigned gpio, unsigned mode);
extern int myGpioRead(unsigned gpio);
extern void myGpioWrite(unsigned gpio, unsigned level);
extern void myGpioSleep(int seconds, int micros);
extern uint32_t myGpioDelay(uint32_t micros);
extern char *myBuf2Str(unsigned count, char *buf);
extern int myPermit(unsigned gpio);
int gpioWaveTxStart(unsigned wave_mode); /* deprecated */

extern int gpioMaskSet;

/* initialise if not libInitialised */

extern uint64_t gpioMask;

typedef struct
{
   unsigned id;
   unsigned state;
   unsigned request;
   unsigned run_state;
   uint32_t waitBits;
   uint32_t changedBits;
   pthread_t *pthIdp;
   pthread_mutex_t pthMutex;
   pthread_cond_t pthCond;
   cmdScript_t script;
} gpioScript_t;

extern gpioScript_t gpioScript [PI_MAX_SCRIPTS];

#define PI_SCRIPT_FREE     0
#define PI_SCRIPT_RESERVED 1
#define PI_SCRIPT_IN_USE   2
#define PI_SCRIPT_DYING    3

#define PI_SCRIPT_HALT   0
#define PI_SCRIPT_RUN    1
#define PI_SCRIPT_DELETE 2

extern volatile uint32_t* gpioReg;


#define GPSET0     7
#define GPSET1     8

#define GPCLR0    10
#define GPCLR1    11

#define GPLEV0    13
#define GPLEV1    14

#define GPEDS0    16
#define GPEDS1    17

#define GPREN0    19
#define GPREN1    20
#define GPFEN0    22
#define GPFEN1    23
#define GPHEN0    25
#define GPHEN1    26
#define GPLEN0    28
#define GPLEN1    29
#define GPAREN0   31
#define GPAREN1   32
#define GPAFEN0   34
#define GPAFEN1   35

#define GPPUD     37
#define GPPUDCLK0 38
#define GPPUDCLK1 39

extern void spinWhileStarting(void);

#define PI_NOTIFY_CLOSED  0
#define PI_NOTIFY_CLOSING 1
#define PI_NOTIFY_OPENED  2
#define PI_NOTIFY_RUNNING 3
#define PI_NOTIFY_PAUSED  4
#define MAX_EMITS (PIPE_BUF / sizeof(gpioReport_t))

typedef struct
{
   uint16_t seqno;
   uint16_t state;
   uint32_t bits;
   uint32_t lastReportTick;
   int      fd;
   int      pipe;
   int      max_emits;
} gpioNotify_t;

extern gpioNotify_t     gpioNotify [PI_NOTIFY_SLOTS];

extern volatile uint32_t alertBits;
extern volatile uint32_t monitorBits;
extern volatile uint32_t notifyBits;
extern volatile uint32_t scriptBits;
extern volatile uint32_t gFilterBits;
extern volatile uint32_t nFilterBits;
extern volatile uint32_t wdogBits;

void intNotifyBits(void);

typedef void (*callbk_t) ();

typedef struct
{
   callbk_t func;
   unsigned ex;
   void *userdata;
   uint32_t bits;
} gpioGetSamples_t;

extern gpioGetSamples_t gpioGetSamples;

#endif //PRIVATE_H
