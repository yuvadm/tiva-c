/* ptpd_dep.h */

#ifndef PTPD_DEP_H
#define PTPD_DEP_H

#include <string.h>

/*
 * Remove the following #if 0 and its matching #endif to enable PTPd debug
 * output on UART0. This is disabled by default even in debug builds due to
 * the volume of information transmitted.
 */
#if 0
#ifdef DEBUG
#include "utils/uartstdio.h"
#define PTPD_DBG
/*
 * Define the following to enable verbose debug.
 */
#undef PTPD_DBGV
#endif
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif
#ifndef size_t
#define size_t long
#endif
#ifndef ssize_t
#define ssize_t long
#endif

/* system messages */
#ifndef PTPD_DBG
#define ERROR(...)
#define PERROR(...)
#define NOTIFY(...)
#else
#define ERROR(x, ...)  UARTprintf("(ptpd error) " x, ##__VA_ARGS__)
#define PERROR(x, ...) UARTprintf("(ptpd error) " x ": %m\n", ##__VA_ARGS__)
#define NOTIFY(x, ...) UARTprintf("(ptpd notice) " x, ##__VA_ARGS__)
#endif

/* debug messages */
#ifdef PTPD_DBGV
#define PTPD_DBG
#define DBGV(x, ...) UARTprintf("(ptpd debug) " x, ##__VA_ARGS__)
#else
#define DBGV(...)
#endif

#ifdef PTPD_DBG
#define DBG(x, ...)  UARTprintf("(ptpd debug) " x, ##__VA_ARGS__)
#else
#define DBG(...)
#endif

/* endian corrections */
#if defined(PTPD_MSBF)
#define shift8(x,y)   ( (x) << ((3-y)<<3) )
#define shift16(x,y)  ( (x) << ((1-y)<<4) )
#define flip16(x)       (x)
#define flip32(x)       (x)

#elif defined(PTPD_LSBF)
#define shift8(x,y)   ( (x) << ((y)<<3) )
#define shift16(x,y)  ( (x) << ((y)<<4) )
#define flip16(x)     ((((x) >> 8) & 0x00ff) | (((x) << 8) & 0xff00))
#define flip32(x)     ((((x) >> 24) & 0x000000ff) | \
                       (((x) >> 8 ) & 0x0000ff00) | \
                       (((x) << 8 ) & 0x00ff0000) | \
                       (((x) << 24) & 0xff000000))
#endif


/* bit array manipulation */
#define getFlag(x,y)  !!( *(UInteger8*)((x)+((y)<8?1:0)) &   (1<<((y)<8?(y):(y)-8)) )
#define setFlag(x,y)    ( *(UInteger8*)((x)+((y)<8?1:0)) |=   1<<((y)<8?(y):(y)-8)  )
#define clearFlag(x,y)  ( *(UInteger8*)((x)+((y)<8?1:0)) &= ~(1<<((y)<8?(y):(y)-8)) )


#define labs(x)         (((x) >= 0) ? (x) : -(x))

/* msg.c */
Boolean msgPeek(char*,size_t);
void msgUnpackHeader(char*,MsgHeader*);
void msgUnpackSync(char*,MsgSync*);
void msgUnpackDelayReq(char*,MsgDelayReq*);
void msgUnpackFollowUp(char*,MsgFollowUp*);
void msgUnpackDelayResp(char*,MsgDelayResp*);
void msgUnpackManagement(char*,MsgManagement*);
UInteger8 msgUnloadManagement(char*,MsgManagement*,PtpClock*,RunTimeOpts*);
void msgUnpackManagementPayload(char *buf, MsgManagement *manage);
void msgPackHeader(char*,PtpClock*);
void msgPackSync(char*,Boolean,TimeRepresentation*,PtpClock*);
void msgPackDelayReq(char*,Boolean,TimeRepresentation*,PtpClock*);
void msgPackFollowUp(char*,UInteger16,TimeRepresentation*,PtpClock*);
void msgPackDelayResp(char*,MsgHeader*,TimeRepresentation*,PtpClock*);
UInteger16 msgPackManagement(char*,MsgManagement*,PtpClock*);
UInteger16 msgPackManagementResponse(char*,MsgHeader*,MsgManagement*,PtpClock*);

/* net.c */
Boolean netInit(NetPath*,RunTimeOpts*,PtpClock*);
Boolean netShutdown(NetPath*);
int netSelect(TimeInternal*,NetPath*);
size_t netRecvEvent(Octet*,TimeInternal*,NetPath*);
size_t netRecvGeneral(Octet*,NetPath*);
size_t netSendEvent(Octet*,UInteger16,NetPath*);
size_t netSendGeneral(Octet*,UInteger16,NetPath*);

/* servo.c */
void initClock(RunTimeOpts*,PtpClock*);
void updateDelay(TimeInternal*,TimeInternal*,
  one_way_delay_filter*,RunTimeOpts*,PtpClock*);
void updateOffset(TimeInternal*,TimeInternal*,
  offset_from_master_filter*,RunTimeOpts*,PtpClock*);
void updateClock(RunTimeOpts*,PtpClock*);

/* sys.c */
void displayStats(RunTimeOpts*,PtpClock*);
Boolean nanoSleep(TimeInternal*);
void getTime(TimeInternal*);
void setTime(TimeInternal*);
UInteger16 getRand(UInteger32*);
Boolean adjFreq(Integer32);

/* timer.c */
void initTimer(void);
void timerTick(int);
void timerUpdate(IntervalTimer*);
void timerStop(UInteger16,IntervalTimer*);
void timerStart(UInteger16,UInteger16,IntervalTimer*);
Boolean timerExpired(UInteger16,IntervalTimer*);


#endif

