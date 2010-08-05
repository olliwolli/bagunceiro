#ifndef _Z_DAY_ENTRY_H
#define _Z_DAY_ENTRY_H

#include <stddef.h>

#include <array.h>
#include <caldate.h>
#include <caltime.h>
#include <taia.h>

#include "z_cdbb.h"

typedef struct day {
	/* array of nentries */
	array es;
	struct caltime time;
} day_entry_t;

void day_init(struct day *d);
size_t day_length(struct day *d);
struct nentry *day_get_nentry(struct day *d, int i);
void day_add_nentry(struct day *d, struct nentry *n);
struct day *day_new();
void day_free(struct day *d);

int cdbb_fetch_day(struct cdbb *a, struct day *entries, const struct taia *day);
#endif
