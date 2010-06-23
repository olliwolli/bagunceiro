#ifndef _Z_CDB_H
#define _Z_CDB_H

#include <array.h>
#include <cdb.h>
#include <cdb_make.h>
#include "z_entry.h"
#include "z_features.h"

#ifdef ADMIN_MODE
int cdb_init_file(const char * file);
int cdb_add(const char * name, const char * k, const size_t ks, const char * v, const size_t vs);
int cdb_del(const char * name, const char * k, const size_t ks);
int cdb_mod(const char * name, const char * k, const size_t ks, const char * v, const size_t vs);
#endif

void dump_array(array * a, size_t es);
int _cdb_get(struct cdb *result, char *key, size_t ks, struct nentry * v);


inline struct cdb * cdb_open_read(const char * file);
inline struct cdb_make * cdb_open_write(const char * file);
inline void cdb_close(struct cdb * result);
#endif
