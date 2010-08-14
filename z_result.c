#include <string.h>

#include "z_result.h"
#include "z_day.h"

void result_init(struct result *r) {
	memset(&r->r, 0, sizeof(array));
}

// not needed
//struct result * result_new()
//{
//	struct result r = malloc(sizeof(struct result));
//	memset(&r->r, 0, sizeof(struct result));
//	return r;
//}

void result_reset(struct result * r) {
	int i;
	struct day * de;
	int n_de;

	n_de = result_length(r);
	for (i = 0; i < n_de; i++) {
		de = result_get_day_entry(r, i);
		day_free(de);
	}
	array_reset(&r->r);
}

size_t result_length(struct result * r) {
	return array_length(&r->r, sizeof(struct day**));
}

void result_add_day_entry(struct result *r, struct day *d) {
	array_catb(&r->r, (char *) &d, sizeof(struct day **));
}

struct day * result_get_day_entry(struct result *r, int i) {
	return *(struct day **) array_get(&r->r, sizeof(struct day **), i);
}
