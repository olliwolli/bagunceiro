#ifndef _Z_ENTRY_H
#define _Z_ENTRY_H

#include <array.h>
#include <taia.h>
#include <caldate.h>

#include "cdbextra.h"

#define MAX_ENTRY_SIZE 32768
#define MAX_ENTRIES_PER_DAY    128

#define new_nentry() malloc(sizeof(struct nentry))
typedef struct nentry {
	array e;		/*  entry */
	struct taia k;		/*  key */
} nentry_t;

typedef struct day_entry {
	array *es;
	struct caldate *date;
} day_entry_t;

int modify_entry(const char *dbpath, struct nentry *entry);
int add_entry_now(const char *dbpath, struct nentry *entry);
int add_entry(const char *dbpath, struct nentry *entry);
int show_entry(const char *dbpath, struct nentry *entry);
int delete_entry(const char *dbpath, struct nentry *entry);

int show_day(const char *dbpath, array * entries, const struct taia *tday);
int show_file(array * entries, const char *file);
void dump_entries(array * entries);
void entry_dump(const struct nentry *e);

void e_add_key(void *e, unsigned char *s, size_t l);
void e_add_val(void *e, unsigned char *s, size_t l);
void e_add_to_array(void *e, array * arr);
void *e_malloc();

void choose_file(array * file, const char *dbpath, const struct taia *key);
#endif
