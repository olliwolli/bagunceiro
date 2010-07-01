#ifndef _Z_DAY_ENTRY_H
#define _Z_DAY_ENTRY_H
#include <array.h>
#include <caldate.h>
#include <caltime.h>

typedef struct day_entry {
	/* array of nentries */
	array es;
	struct caltime time;
} day_entry_t;

struct day_entry * new_day_entry();
void free_day_entry(struct day_entry * d);
int _show_day(struct cdb * result, array * entries, const struct taia * day);
#endif
