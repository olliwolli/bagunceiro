#ifndef _Z_TIME_H
#define _Z_TIME_H

#include <sys/time.h>
#include <taia.h>
#include <array.h>
#include <caldate.h>
#include <caltime.h>
#include "z_features.h"

/* these functions reduce the time stamp for presentation purposes
 * they will break on 2106-02-07 by overflowing after this reduction:
 * 40000000ffffffffffffffff00000000 -> ffffffffffffffff
 * If you want to be safe for the future, increase REDUCE_SIZE */

/* MAX is 24 */
#define REDUCE_SIZE 16
#define REDUCE_OFFSET (24 - REDUCE_SIZE)

void reduce_ts(char * src);
void inflate_ts(char * src);

/* 14 is needed for the following:* Mon 5 Jul 2010, 18
 *  will work until the year 99999999 */
#define FMT_TAIA_STR 18
size_t fmt_time_str(char * s, const struct taia *time);

#define FMT_TAIA_HEX TAIA_PACK * 2 + 1
size_t fmt_time_hex(char * s, const struct taia *time);

/* We use length 15 for strings formatted by the fmt_caldate
 * function. This will work until the year 99999999.
 * To prevent overflows in usage use constructs like
 * if(caldate_fmt(0, &date) < FMT_CALDATE) */
#define FMT_CALDATE	15

/* simple date format like: 2010-06-31 : 10
 * 14 will work until 99999999 */
#define FMT_CALDATE_NAV 14
unsigned int fmt_caldate_nav(char *s, const struct caldate *cd);

/* scans ascii hex encoded taia */
size_t scan_time_hex(const char * s, struct taia *time);
size_t sub_days(struct taia *time, const unsigned int days);

/* for benchmarking */
void time_stop_print(struct timeval *time);
#endif
