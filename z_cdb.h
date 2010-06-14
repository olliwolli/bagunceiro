#ifndef _Z_CDB_H
#define _Z_CDB_H

#include <array.h>

int cdb_init_file(array * file);
typedef struct eops {
	void (*add_key) (void *e, unsigned char *s, size_t l);
	void *(*alloc) ();
	void (*add_val) (void *e, unsigned char *s, size_t l);
	void (*add_to_array) (void *e, array * arr);
} eops_t;

int cdb_read_all(const char *name, array * entries, struct eops *ops);

int cdb_add(const array * name, const array * k, const array * v);
int cdb_del(const array * name, const array * k);
int cdb_mod(const array * name, const array * k, const array * v);
int cdb_get(const array * name, const array * k, array * v);
#endif
