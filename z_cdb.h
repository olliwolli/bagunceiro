#ifndef _Z_CDB_H
#define _Z_CDB_H

#include <array.h>
#include <cdb.h>
#include <cdb_make.h>
#include "z_entry.h"
#include "z_features.h"

#define CDB_DO_COPY 0
#define CDB_DO_NOT_COPY 1

	/* ten should be ok for now, if not you can increase
	 * that or implement it as with the dynamic array
	 * interface */
#define NUM_CALLBACKS 10

typedef struct cdb_action{
	struct cdb_make *w;
	struct cdb *r;
	char *f; /* file to read from */
	char *t; /* file to write to */
	char *k;
	size_t ks;
	char *v;
	size_t vs;
	struct nentry e;
	unsigned char *needle[NUM_CALLBACKS];
	int (*cbs[NUM_CALLBACKS])(struct cdb_action * a, int * copied);
	int cbsnum;
} t_cdb_action;

#ifdef ADMIN_MODE
int cdb_init_file(const char * file);
int cdb_add(const char * name, const char * k, const size_t ks, const char * v, const size_t vs);
int cdb_del(const char * name, const char * k, const size_t ks);
int cdb_add_idx(const char * name, const char * k, const size_t ks, const char * v, const size_t vs);
int cdb_del_idx(const char * name, const char * k, const size_t ks);
int cdb_mod(const char * name, const char * k, const size_t ks, const char * v, const size_t vs);
#endif

void dump_array(array * a, size_t es);
int _cdb_get(struct cdb *result, char *key, size_t ks, struct nentry *v);
inline int _cdb_get_value(const char * file, char *key, size_t ks, struct nentry *v);

inline struct cdb * cdb_open_read(const char * file);
inline struct cdb_make * cdb_open_write(const char * file);
inline void cdbm_make_close(struct cdb_make *cdbm);
inline void cdb_close(struct cdb * result);
#endif
