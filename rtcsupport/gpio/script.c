#include "pigpio.h"
#include "private.h"
#include <stdlib.h>

#define PI_SCRIPT_STACK_SIZE 256

gpioScript_t     gpioScript [PI_MAX_SCRIPTS];

static int scrPop(gpioScript_t *s, int *SP, int *S)
{
   if ((*SP) > 0)
   {
      return S[--(*SP)];
   }
   else
   {
      s->run_state = PI_SCRIPT_FAILED;
      DBG(DBG_ALWAYS, "script %d too many pops", s->id);
      return 0;
   }
}

/* ----------------------------------------------------------------------- */

static void scrPush(gpioScript_t *s, int *SP, int *S, int val)
{
   if ((*SP) < PI_SCRIPT_STACK_SIZE)
   {
      S[(*SP)++] = val;
   }
   else
   {
      s->run_state = PI_SCRIPT_FAILED;
      DBG(DBG_ALWAYS, "script %d too many pushes", s->id);
   }
}

/* ----------------------------------------------------------------------- */

static void scrSwap(int *v1, int *v2)
{
   int t;

   t=*v1; *v1=*v2; *v2= t;
}

static void intScriptBits(void)
{
   int i;
   uint32_t bits;

   bits = 0;

   for (i=0; i<PI_MAX_SCRIPTS; i++)
   {
      if (gpioScript[i].state == PI_SCRIPT_IN_USE)
      {
         bits |= gpioScript[i].waitBits;
      }
   }

   scriptBits = bits;

   monitorBits = alertBits | notifyBits | scriptBits | gpioGetSamples.bits;
}

/* ----------------------------------------------------------------------- */

static int scrWait(gpioScript_t *s, uint32_t bits)
{
   pthread_mutex_lock(&s->pthMutex);

   if (s->request == PI_SCRIPT_RUN)
   {
      s->run_state = PI_SCRIPT_WAITING;
      s->waitBits = bits;
      intScriptBits();

      pthread_cond_wait(&s->pthCond, &s->pthMutex);

      s->waitBits = 0;
      intScriptBits();
      s->run_state = PI_SCRIPT_RUNNING;
   }

   pthread_mutex_unlock(&s->pthMutex);

   return s->changedBits;
}

/* ----------------------------------------------------------------------- */

extern int myDoCommand(uint32_t *p, unsigned bufSize, char *buf);

static int scrSys(char *cmd, uint32_t p1, uint32_t p2)
{
   char buf[256];
   char pars[40];

   sprintf(pars, " %u %u", p1, p2);
   strcpy(buf, "/opt/pigpio/cgi/");
   strncat(buf, cmd, 200);
   strcat(buf, pars);

   DBG(DBG_USER, "sys %s", buf);

   return system(buf);
}

static void *pthScript(void *x)
{
   gpioScript_t *s;
   cmdInstr_t instr;
   int p1, p2, p1o, p2o, *t1, *t2;
   int PC, A, F, SP;
   int S[PI_SCRIPT_STACK_SIZE];
   char buf[CMD_MAX_EXTENSION];


   S[0] = 0; /* to prevent compiler warning */

   s = x;

   while ((volatile int)s->request != PI_SCRIPT_DELETE)
   {
      pthread_mutex_lock(&s->pthMutex);
      s->run_state = PI_SCRIPT_HALTED;
      pthread_cond_wait(&s->pthCond, &s->pthMutex);
      pthread_mutex_unlock(&s->pthMutex);

      s->run_state = PI_SCRIPT_RUNNING;

      A  = 0;
      F  = 0;
      PC = 0;
      SP = 0;

      while (((volatile int)s->request   == PI_SCRIPT_RUN    ) &&
                           (s->run_state == PI_SCRIPT_RUNNING))
      {
         instr = s->script.instr[PC];

         p1o = instr.p[1];
         p2o = instr.p[2];

         if      (instr.opt[1] == CMD_VAR) instr.p[1] = s->script.var[p1o];
         else if (instr.opt[1] == CMD_PAR) instr.p[1] = s->script.par[p1o];

         if      (instr.opt[2] == CMD_VAR) instr.p[2] = s->script.var[p2o];
         else if (instr.opt[2] == CMD_PAR) instr.p[2] = s->script.par[p2o];
/*
         fprintf(stderr, "PC=%d cmd=%d p1o=%d p1=%d p2o=%d p2=%d\n",
            PC, instr.p[0], p1o, instr.p[1], p2o, instr.p[2]);
         fflush(stderr);
*/
         if (instr.p[0] < 100)
         {
            if (instr.p[3])
            {
               memcpy(buf, (char *)instr.p[4], instr.p[3]);
            }

            A = myDoCommand(instr.p, sizeof(buf)-1, buf);

            F = A;

            PC++;
         }
         else
         {
            p1 = instr.p[1];
            p2 = instr.p[2];

            switch (instr.p[0])
            {
               case PI_CMD_ADD:   A+=p1; F=A;                     PC++; break;

               case PI_CMD_AND:   A&=p1; F=A;                     PC++; break;

               case PI_CMD_CALL:  scrPush(s, &SP, S, PC+1);    PC = p1; break;

               case PI_CMD_CMP:   F=A-p1;                         PC++; break;

               case PI_CMD_DCR:
                  if (instr.opt[1] == CMD_PAR)
                     {--s->script.par[p1o]; F=s->script.par[p1o];}
                  else
                     {--s->script.var[p1o]; F=s->script.var[p1o];}
                  PC++;
                  break;

               case PI_CMD_DCRA:  --A; F=A;                       PC++; break;

               case PI_CMD_DIV:   A/=p1; F=A;                     PC++; break;

               case PI_CMD_HALT: s->run_state = PI_SCRIPT_HALTED;       break;

               case PI_CMD_INR:
                  if (instr.opt[1] == CMD_PAR)
                     {++s->script.par[p1o]; F=s->script.par[p1o];}
                  else
                     {++s->script.var[p1o]; F=s->script.var[p1o];}
                  PC++;
                  break;

               case PI_CMD_INRA:  ++A; F=A;                       PC++; break;

               case PI_CMD_JM:    if (F<0)  PC=p1; else PC++;           break;

               case PI_CMD_JMP:   PC=p1;                                break;

               case PI_CMD_JNZ:   if (F)    PC=p1; else PC++;           break;

               case PI_CMD_JP:    if (F>=0) PC=p1; else PC++;           break;

               case PI_CMD_JZ:    if (!F)   PC=p1; else PC++;           break;

               case PI_CMD_LD:
                  if (instr.opt[1] == CMD_PAR) s->script.par[p1o]=p2;
                  else                         s->script.var[p1o]=p2;
                  PC++;
                  break;

               case PI_CMD_LDA:   A=p1;                           PC++; break;

               case PI_CMD_LDAB:
                  if ((p1 >= 0) && (p1 < sizeof(buf))) A = buf[p1];
                  PC++;
                  break;

               case PI_CMD_MLT:   A*=p1; F=A;                     PC++; break;

               case PI_CMD_MOD:   A%=p1; F=A;                     PC++; break;

               case PI_CMD_OR:    A|=p1; F=A;                     PC++; break;

               case PI_CMD_POP:
                  if (instr.opt[1] == CMD_PAR)
                     s->script.par[p1o]=scrPop(s, &SP, S);
                  else
                     s->script.var[p1o]=scrPop(s, &SP, S);
                  PC++;
                  break;

               case PI_CMD_POPA:  A=scrPop(s, &SP, S);            PC++; break;

               case PI_CMD_PUSH:
                  if (instr.opt[1] == CMD_PAR)
                     scrPush(s, &SP, S, s->script.par[p1o]);
                  else
                     scrPush(s, &SP, S, s->script.var[p1o]);
                  PC++;
                  break;

               case PI_CMD_PUSHA: scrPush(s, &SP, S, A);          PC++; break;

               case PI_CMD_RET:   PC=scrPop(s, &SP, S);                 break;

               case PI_CMD_RL:
                  if (instr.opt[1] == CMD_PAR)
                     {s->script.par[p1o]<<=p2; F=s->script.par[p1o];}
                  else
                     {s->script.var[p1o]<<=p2; F=s->script.var[p1o];}
                  PC++;
                  break;

               case PI_CMD_RLA:   A<<=p1; F=A;                    PC++; break;

               case PI_CMD_RR:
                  if (instr.opt[1] == CMD_PAR)
                     {s->script.par[p1o]>>=p2; F=s->script.par[p1o];}
                  else
                     {s->script.var[p1o]>>=p2; F=s->script.var[p1o];}
                  PC++;
                  break;

               case PI_CMD_RRA:   A>>=p1; F=A;                    PC++; break;

               case PI_CMD_STA:
                  if (instr.opt[1] == CMD_PAR) s->script.par[p1o]=A;
                  else                         s->script.var[p1o]=A;
                  PC++;
                  break;

               case PI_CMD_STAB:
                  if ((p1 >= 0) && (p1 < sizeof(buf))) buf[p1] = A;
                  PC++;
                  break;

               case PI_CMD_SUB:   A-=p1; F=A;                     PC++; break;

               case PI_CMD_SYS:
                  A=scrSys((char*)instr.p[4], A, *(gpioReg + GPLEV0));
                  F=A;
                  PC++;
                  break;

               case PI_CMD_WAIT:  A=scrWait(s, p1); F=A;          PC++; break;

               case PI_CMD_X:
                  if (instr.opt[1] == CMD_PAR) t1 = &s->script.par[p1o];
                  else                         t1 = &s->script.var[p1o];

                  if (instr.opt[2] == CMD_PAR) t2 = &s->script.par[p2o];
                  else                         t2 = &s->script.var[p2o];

                  scrSwap(t1, t2);
                  PC++;
                  break;

               case PI_CMD_XA:
                  if (instr.opt[1] == CMD_PAR)
                     scrSwap(&s->script.par[p1o], &A);
                  else
                     scrSwap(&s->script.var[p1o], &A);
                  PC++;
                  break;

               case PI_CMD_XOR:   A^=p1; F=A;                     PC++; break;

            }
         }

         if (PC >= s->script.instrs) s->run_state = PI_SCRIPT_HALTED;

      }

      if ((volatile int)s->request == PI_SCRIPT_HALT)
         s->run_state = PI_SCRIPT_HALTED;

   }

   return 0;
}

int gpioStoreScript(char *script)
{
   gpioScript_t *s;
   int status, slot, i;

   DBG(DBG_USER, "script=[%s]", script);

   CHECK_INITED;

   slot = -1;

   for (i=0; i<PI_MAX_SCRIPTS; i++)
   {
      if (gpioScript[i].state == PI_SCRIPT_FREE)
      {
         gpioScript[i].state = PI_SCRIPT_RESERVED;
         slot = i;
         break;
      }
   }

   if (slot < 0)
      SOFT_ERROR(PI_NO_SCRIPT_ROOM, "no room for scripts");

   s = &gpioScript[slot];

   status = cmdParseScript(script, &s->script, 0);

   if (status == 0)
   {
      s->request   = PI_SCRIPT_HALT;
      s->run_state = PI_SCRIPT_INITING;

      pthread_cond_init(&s->pthCond, NULL);
      pthread_mutex_init(&s->pthMutex, NULL);

      s->id = slot;

      gpioScript[slot].state = PI_SCRIPT_IN_USE;

      s->pthIdp = gpioStartThread(pthScript, s);

      status = slot;

   }
   else
   {
      if (s->script.par)
          free(s->script.par);
      s->script.par = NULL;
      gpioScript[slot].state = PI_SCRIPT_FREE;
   }

   return status;
}

/* ----------------------------------------------------------------------- */

int gpioRunScript(unsigned script_id, unsigned numParam, uint32_t *param)
{
   int status = 0;

   DBG(DBG_USER, "script_id=%d numParam=%d param=%08X",
      script_id, numParam, (uint32_t)param);

   CHECK_INITED;

   if (script_id >= PI_MAX_SCRIPTS)
      SOFT_ERROR(PI_BAD_SCRIPT_ID, "bad script id(%d)", script_id);

   if (numParam > PI_MAX_SCRIPT_PARAMS)
      SOFT_ERROR(PI_TOO_MANY_PARAM, "bad number of parameters(%d)", numParam);

   if (gpioScript[script_id].state == PI_SCRIPT_IN_USE)
   {
      pthread_mutex_lock(&gpioScript[script_id].pthMutex);

      if (gpioScript[script_id].run_state == PI_SCRIPT_HALTED)
      {
         if ((numParam > 0) && (param != 0))
         {
            memcpy(gpioScript[script_id].script.par, param,
               sizeof(uint32_t) * numParam);
         }

         gpioScript[script_id].request = PI_SCRIPT_RUN;

         pthread_cond_signal(&gpioScript[script_id].pthCond);
      }
      else
      {
         status = PI_NOT_HALTED;
      }

      pthread_mutex_unlock(&gpioScript[script_id].pthMutex);

      return status;
   }
   else
   {
      return PI_BAD_SCRIPT_ID;
   }
}


/* ----------------------------------------------------------------------- */

int gpioScriptStatus(unsigned script_id, uint32_t *param)
{
   DBG(DBG_USER, "script_id=%d param=%08X", script_id, (uint32_t)param);

   CHECK_INITED;

   if (script_id >= PI_MAX_SCRIPTS)
      SOFT_ERROR(PI_BAD_SCRIPT_ID, "bad script id(%d)", script_id);

   if (gpioScript[script_id].state == PI_SCRIPT_IN_USE)
   {
      if (param != NULL)
      {
         memcpy(param, gpioScript[script_id].script.par,
            sizeof(uint32_t) * PI_MAX_SCRIPT_PARAMS);
      }

      return gpioScript[script_id].run_state;
   }

    return PI_BAD_SCRIPT_ID;
}


/* ----------------------------------------------------------------------- */

int gpioStopScript(unsigned script_id)
{
   DBG(DBG_USER, "script_id=%d", script_id);

   CHECK_INITED;

   if (script_id >= PI_MAX_SCRIPTS)
      SOFT_ERROR(PI_BAD_SCRIPT_ID, "bad script id(%d)", script_id);

   if (gpioScript[script_id].state == PI_SCRIPT_IN_USE)
   {
      pthread_mutex_lock(&gpioScript[script_id].pthMutex);

      gpioScript[script_id].request = PI_SCRIPT_HALT;

      if (gpioScript[script_id].run_state == PI_SCRIPT_WAITING)
      {
         pthread_cond_signal(&gpioScript[script_id].pthCond);
      }

      pthread_mutex_unlock(&gpioScript[script_id].pthMutex);

      return 0;
   }
   else return PI_BAD_SCRIPT_ID;
}

/* ----------------------------------------------------------------------- */

int gpioDeleteScript(unsigned script_id)
{
   DBG(DBG_USER, "script_id=%d", script_id);

   CHECK_INITED;

   if (script_id >= PI_MAX_SCRIPTS)
      SOFT_ERROR(PI_BAD_SCRIPT_ID, "bad script id(%d)", script_id);

   if (gpioScript[script_id].state == PI_SCRIPT_IN_USE)
   {
      gpioScript[script_id].state = PI_SCRIPT_DYING;

      pthread_mutex_lock(&gpioScript[script_id].pthMutex);

      gpioScript[script_id].request = PI_SCRIPT_HALT;

      if (gpioScript[script_id].run_state == PI_SCRIPT_WAITING)
      {
         pthread_cond_signal(&gpioScript[script_id].pthCond);
      }

      pthread_mutex_unlock(&gpioScript[script_id].pthMutex);

      while (gpioScript[script_id].run_state == PI_SCRIPT_RUNNING)
      {
         myGpioSleep(0, 5000); /* give script time to halt */
      }

      gpioStopThread(gpioScript[script_id].pthIdp);

      if (gpioScript[script_id].script.par)
         free(gpioScript[script_id].script.par);

      gpioScript[script_id].script.par = NULL;

      gpioScript[script_id].state = PI_SCRIPT_FREE;

      return 0;
   }
   else return PI_BAD_SCRIPT_ID;
}

