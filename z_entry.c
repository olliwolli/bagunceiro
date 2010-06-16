#include <stdlib.h>

#include <open.h>
#include <cdb.h>
#include <str.h>
#include <taia.h>
#include <array.h>
#include <byte.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_features.h"

inline static void choose_file(array * file, const char *dbpath, const struct taia *key)
{
	int len;
	char buf[MAX_FMT_LENGTH_KEY];
	byte_zero(&buf, sizeof(buf));

#ifdef USE_COHERENT_TIME
#include <caltime.h>
	char dfmt[20];
	struct caltime cd;
	caltime_utc(&cd, &key->sec, (int *)0, (int *)0);
	dfmt[caldate_fmtn(dfmt, &cd.date)] = 0;
	array_cats(file, dbpath);
	array_cats0(file, dfmt);
#else
	len = fmt_time_str(buf, key);
	array_cats(file, dbpath);
	array_cats0(file, buf);
#endif
}

// if not exists ..
#define CHOOSE_DB_FILE(f,a,e) \
	array (f); \
	byte_zero(&(f), sizeof((f))); \
	choose_file(&(f), (a), (&e->k)); \
	cdb_init_file((f).p);

#ifdef ADMIN_MODE
int add_entry(const char *dbpath, struct nentry *entry)
{
	int err;

	char pk[TAIA_PACK];
	taia_pack(pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	err = cdb_add(dbfile.p, pk, TAIA_PACK, &entry->e);

	array_cat0(&entry->e);

	return err;
}

int add_entry_now(const char *dbpath, struct nentry *entry)
{
	taia_now(&entry->k);

	return add_entry(dbpath, entry);
}

int modify_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pk[TAIA_PACK];
	taia_pack(pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	err = cdb_mod(dbfile.p, pk, TAIA_PACK, &entry->e);

	return err;
}

int delete_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pk[TAIA_PACK];

	taia_pack(pk, &entry->k);
	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	err = cdb_del(dbfile.p, pk, TAIA_PACK);

	return err;
}
#endif
#ifdef DEBUG_ENTRY

void entry_dump(const struct nentry *e)
{
	static array b;

	fmt_time_hex(&b, &e->k);
	array_cat0(&b);
	sprintm("Dump: ", b.p, "->", e->e.p, "\n");
	print_time(&e->k);
	array_reset(&b);
}

void dump_entries(array * entries)
{
	int i, len;
	struct nentry *tmp;
	char efmt[FMT_ULONG];

	len = array_length(entries, sizeof(struct nentry));
	efmt[fmt_int(efmt, len)] = 0;
	sprintmf("Found ", efmt, " entries\n");
	for (i = 0; i < len; i++) {
		tmp = (struct nentry *)array_get(entries, sizeof(struct nentry),
			i);
		entry_dump(tmp);
	}
}
#endif

int show_entry(const char *dbpath, struct nentry *entry)
{
	struct cdb result;
	int err, fd, len;
	static array dbfile;
	char hkey[MAX_FMT_BIN_LENGTH_KEY];

	char pac[TAIA_PACK];
	unsigned char buf[MAX_ENTRY_SIZE];

	byte_zero(&dbfile, sizeof(array));

	taia_pack(pac, &entry->k);
	choose_file(&dbfile, dbpath, &entry->k);

	fd = open_read(dbfile.p);
	if (fd <= 0) {
		eprintmf("File not found", dbfile.p, "\n");
		return -1;
	}

	cdb_init(&result, fd);
	err = cdb_find(&result, (unsigned char *)pac, TAIA_PACK);

	fmt_time_hex(hkey, &entry->k);

	if (err == 0) {
		eprintmf("Key ", hkey, "not found\n");
		return 0;
	} else if (err == -1) {
		eprintmf("Read error when trying to find key ", hkey, "\n");
		return -1;
	} else {
		len = cdb_datalen(&result);
		err = cdb_read(&result, buf, len, cdb_datapos(&result));
		if (err < 0) {
			eprintmf("Error reading entry ", hkey, "\n");
		}

		array_catb(&entry->e, (char *)buf, len);
		return 1;
	}

	array_reset(&dbfile);
}

void e_add_key(void *e, unsigned char *s, size_t l)
{
	struct nentry *t = (struct nentry *)e;
	taia_unpack((char *)s, &t->k);

#ifdef DEBUG_1
	print_time(&t->k);
	printf("Keylen %d", l);
#endif
}

void e_add_val(void *e, unsigned char *s, size_t l)
{
	struct nentry *t = (struct nentry *)e;

	array_catb(&(t)->e, (const char *)s, l);
	array_cat0(&t->e);
}

void e_add_to_array(void *e, array * arr)
{
	struct nentry *t = (struct nentry *)e;

	array_catb(arr, (char *)t, sizeof(struct nentry));
}

void *e_malloc()
{
	struct nentry *tmp;

	tmp = malloc(sizeof(struct nentry));
	byte_zero(&tmp->e, sizeof(array));

	return tmp;
}

int show_day(const char *dbpath, array * entries, const struct taia *day)
{

	array key, dbfile;
	struct eops ops;
	int err;

	ops.add_key = e_add_key;
	ops.add_val = e_add_val;
	ops.add_to_array = e_add_to_array;
	ops.alloc = e_malloc;

	byte_zero(&dbfile, sizeof(dbfile));
	byte_zero(&key, sizeof(key));
	choose_file(&dbfile, dbpath, day);
	err = cdb_read_all(dbfile.p, entries, &ops);
	if (err == -2) {
		str_copy(gerr.note, dbfile.p);
		gerr.type = N_ERROR;
		gerr.error = ESPIPE;
	}
	array_reset(&key);
	array_reset(&dbfile);

	return err;
}

int show_file(array * entries, const char *file)
{
	struct eops ops;
	int err;

	ops.add_key = e_add_key;
	ops.add_val = e_add_val;
	ops.add_to_array = e_add_to_array;
	ops.alloc = e_malloc;
	err = cdb_read_all(file, entries, &ops);

	return err;
}
