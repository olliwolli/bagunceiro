#ifndef _Z_ENTRY_H
#define _Z_ENTRY_H

#include <array.h>
#include <taia.h>
#include <cdb.h>

#include "z_cdbb.h"

#define MAX_ENTRY_SIZE 32768
#define MAX_ENTRIES_PER_DAY    128

void add_entry(struct cdbb *a, struct taia *k, char *v, size_t vs);
void add_entry_now(struct cdbb *a, char *v, size_t vs);
void modify_entry(struct cdbb *a, struct taia *k, char *v, size_t vs);
void delete_entry(struct cdbb *a, struct taia *k);
int show_entry(struct cdbb *a, struct taia *k, struct nentry *n);

int show_file(array * entries, const char *file);
void dump_entries(array * entries);
void entry_dump(const struct nentry *e);

void e_add_key(void *e, unsigned char *s, size_t l);
void e_add_val(void *e, unsigned char *s, size_t l);
void e_add_to_array(void *e, array * arr);
void *e_malloc(size_t s);

#endif
