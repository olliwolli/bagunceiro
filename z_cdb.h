#ifndef _Z_CDB_H
#define _Z_CDB_H

#include <array.h>
#include "z_features.h"

typedef struct eops {
	void (*add_key) (void *e, unsigned char *s, size_t l);
	void *(*alloc) ();
	void (*add_val) (void *e, unsigned char *s, size_t l);
	void (*add_to_array) (void *e, array * arr);
} eops_t;

#ifdef ADMIN_MODE
int cdb_init_file(const char * file);
int cdb_add(const char * name, const char * k, const size_t ks, const array * v);
int cdb_del(const char * name, const char * k, const size_t ks);
int cdb_mod(const char * name, const char * k, const size_t ks, const array * v);
#endif

int cdb_read_all(const char *name, array * entries, struct eops *ops);
int cdb_get(const char * name, const char * k, const size_t ks, array * v);

#endif
