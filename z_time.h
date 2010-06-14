#ifndef _Z_TIME_H
#define _Z_TIME_H

#include <sys/time.h>
#include <taia.h>
#include <array.h>
#include <caldate.h>

size_t fmt_time_hex(array * s, const struct taia *time);
size_t fmt_time_str(array * s, const struct taia *time);
size_t fmt_time_tstr(array * s, const struct taia *time);
size_t fmt_time_bin(array * s, const struct taia *time);

size_t scan_time_hex(const array * s, struct taia *time);
size_t scan_time_tstr(const array * s, struct taia *time);

size_t print_time(const struct taia *time);

size_t ht_sub_days(struct taia *time, const unsigned int days);
unsigned int caldate_fmtn(char *s, struct caldate *cd);
/* for benchmarking */
void time_start(struct timeval *time);
void time_stop_print(struct timeval *time);
#endif
