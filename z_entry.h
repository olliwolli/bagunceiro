#ifndef _Z_ENTRY_H
#define _Z_ENTRY_H

#include <array.h>
#include <taia.h>

#include <cdb.h>

#define MAX_ENTRY_SIZE 32768
#define MAX_ENTRIES_PER_DAY    128

typedef struct nentry {
	array e;		/*  entry */
	struct taia k;		/*  key */
	char *kp;	/*  key packed */
} nentry_t;

struct nentry * new_nentry(void);

int modify_entry(const char *dbpath, struct nentry *entry);
int add_entry_now(const char *dbpath, struct nentry *entry);
int add_entry(const char *dbpath, struct nentry *entry);
int delete_entry(const char *dbpath, struct nentry *entry);

int _show_entry(struct cdb * result, struct nentry *entry);

int show_file(array * entries, const char *file);
void dump_entries(array * entries);
void entry_dump(const struct nentry *e);

void e_add_key(void *e, unsigned char *s, size_t l);
void e_add_val(void *e, unsigned char *s, size_t l);
void e_add_to_array(void *e, array * arr);
void *e_malloc(size_t s);

#endif
