#ifndef _Z_RESULT_H
#define _Z_RESULT_H

#include "z_day.h"

struct result {
	array r;
};

void result_init(struct result *r);
//struct result * result_new();
void result_reset(struct result * r);
size_t result_length(struct result * r);

void result_add_day_entry(struct result *r, struct day *d);
struct day * result_get_day_entry(struct result *r, int i);


#endif
