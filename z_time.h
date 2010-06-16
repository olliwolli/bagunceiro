#ifndef _Z_TIME_H
#define _Z_TIME_H

#include <sys/time.h>
#include <taia.h>
#include <array.h>
#include <caldate.h>

#define MAX_FMT_LENGTH_KEY 33
#define MAX_FMT_HEX_LENGTH_KEY TAIA_PACK * 2
#define MAX_FMT_BIN_LENGTH_KEY TAIA_PACK
size_t fmt_time_hex(char * s, const struct taia *time);
size_t fmt_time_str(char * s, const struct taia *time);
size_t fmt_time_tstr(char * s, const struct taia *time);
//size_t fmt_time_bin(array * s, const struct taia *time);

size_t scan_time_hex(const char * s, struct taia *time);
size_t scan_time_tstr(const char * s, struct taia *time);

size_t print_time(const struct taia *time);

size_t ht_sub_days(struct taia *time, const unsigned int days);
unsigned int caldate_fmtn(char *s, const struct caldate *cd);
/* for benchmarking */
void time_stop_print(struct timeval *time);
#endif
