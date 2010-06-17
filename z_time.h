#ifndef _Z_TIME_H
#define _Z_TIME_H

#include <sys/time.h>
#include <taia.h>
#include <array.h>
#include <caldate.h>

#include "z_features.h"

#define REDUCE_OFFSET 8
#define REDUCE_SIZE 16

#define FMT_TAIA_HEX TAIA_PACK * 2
size_t fmt_time_hex(char * s, const struct taia *time);

/* should be enough for anyone ;-) */
#define FMT_TAIA_STR 33
size_t fmt_time_str(char * s, const struct taia *time);

size_t scan_time_hex(const char * s, struct taia *time);

size_t print_time(const struct taia *time);

size_t ht_sub_days(struct taia *time, const unsigned int days);
unsigned int caldate_fmtn(char *s, const struct caldate *cd);

/* for benchmarking */
void time_stop_print(struct timeval *time);


void reduce_ts(char * src);
void inflate_ts(char * src);

#endif
