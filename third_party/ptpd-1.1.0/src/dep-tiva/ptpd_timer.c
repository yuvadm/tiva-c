/* ptpd_timer.c */

#include "../ptpd.h"

static int iElapsedMilliSeconds = 0;

void initTimer(void)
{
  DBG("initTimer\n");
  
  iElapsedMilliSeconds = 0;
}

void timerTick(int iTickMilliSeconds)
{
    iElapsedMilliSeconds += iTickMilliSeconds;
}

void timerUpdate(IntervalTimer *itimer)
{
  int i, delta;
  
  delta = iElapsedMilliSeconds / 1000;

  if(delta <= 0)
    return;

  iElapsedMilliSeconds %= 1000;

  for(i = 0; i < TIMER_ARRAY_SIZE; ++i)
  {
    if(itimer[i].interval > 0 && (itimer[i].left -= delta) <= 0)
    {
      itimer[i].left = itimer[i].interval;
      itimer[i].expire = TRUE;
      DBGV("timerUpdate: timer %u expired\n", i);
    }
  }
}

void timerStop(UInteger16 index, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].interval = 0;
}

void timerStart(UInteger16 index, UInteger16 interval, IntervalTimer *itimer)
{
  if(index >= TIMER_ARRAY_SIZE)
    return;
  
  itimer[index].expire = FALSE;
  itimer[index].left = interval;
  itimer[index].interval = itimer[index].left;
  
  DBGV("timerStart: set timer %d to %d\n", index, interval);
}

Boolean timerExpired(UInteger16 index, IntervalTimer *itimer)
{
  timerUpdate(itimer);
  
  if(index >= TIMER_ARRAY_SIZE)
    return FALSE;
  
  if(!itimer[index].expire)
    return FALSE;
  
  itimer[index].expire = FALSE;
  
  return TRUE;
}

