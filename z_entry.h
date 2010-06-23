#ifndef _Z_ENTRY_H
#define _Z_ENTRY_H

#include <array.h>
#include <taia.h>
#include <caldate.h>
#include <caltime.h>
#include <cdb.h>

#define MAX_ENTRY_SIZE 32768
#define MAX_ENTRIES_PER_DAY    128

typedef struct nentry {
	array e;		/*  entry */
	struct taia k;		/*  key */
	char *kp;	/*  key packed */
} nentry_t;

typedef struct day_entry {
	/* array of nentries */
	array es;
	struct caltime time;
} day_entry_t;

struct day_entry * new_day_entry();
struct nentry * new_nentry(void);
void free_day_entry(struct day_entry * d);
int modify_entry(const char *dbpath, struct nentry *entry);
int add_entry_now(const char *dbpath, struct nentry *entry);
int add_entry(const char *dbpath, struct nentry *entry);
int delete_entry(const char *dbpath, struct nentry *entry);

int _show_entry(struct cdb * result, struct nentry *entry);
int _show_day(struct cdb * result, array * entries, const struct taia * day);
int show_day(const char *dbpath, array * entries, const struct taia *tday);
int show_file(array * entries, const char *file);
void dump_entries(array * entries);
void entry_dump(const struct nentry *e);

void e_add_key(void *e, unsigned char *s, size_t l);
void e_add_val(void *e, unsigned char *s, size_t l);
void e_add_to_array(void *e, array * arr);
void *e_malloc(size_t s);

#endif
