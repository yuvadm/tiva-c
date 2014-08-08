/**
 * @file   ptpd.h
 * @mainpage Ptpd Documentation
 * @authors Martin Burnicki, Alexandre van Kempen, Steven Kreuzer,
 *          George Neville-Neil
 * @version 1.0
 * @date   Tue Jul 20 16:15:04 2010
 *
 *
 */

#ifndef PTPD_H
#define PTPD_H

/* Definitions and dependencies for the TI TivaWare port */
#include "constants.h"
#include "dep-tiva/constants_dep.h"
#include "dep-tiva/datatypes_dep.h"
#include "datatypes.h"
#include "dep-tiva/ptpd_dep.h"

/* arith.c */
UInteger32 crc_algorithm(Octet *, Integer16);
UInteger32 sum(Octet *, Integer16);
void    fromInternalTime(TimeInternal *, TimeRepresentation *, Boolean);
void    toInternalTime(TimeInternal *, TimeRepresentation *, Boolean *);
void    normalizeTime(TimeInternal *);
void    addTime(TimeInternal *, TimeInternal *, TimeInternal *);
void    subTime(TimeInternal *, TimeInternal *, TimeInternal *);

/* bmc.c */
UInteger8 bmc(ForeignMasterRecord *, RunTimeOpts *, PtpClock *);
void    m1 (PtpClock *);
void    s1 (MsgHeader *, MsgSync *, PtpClock *);
void    initData(RunTimeOpts *, PtpClock *);

/* probe.c */
void    probe(RunTimeOpts *, PtpClock *);

/* protocol.c */
void    protocol(RunTimeOpts *, PtpClock *);

/* Split versions of protocol() that may be used when no RTOS is present.
 * These functions were added by TI.
 */
void protocol_first(RunTimeOpts*,PtpClock*);
void protocol_loop(RunTimeOpts*,PtpClock*);

#endif
