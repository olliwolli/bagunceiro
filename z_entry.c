#include <stdlib.h>

#include <open.h>
#include <cdb.h>

#include <taia.h>
#include <array.h>
#include <byte.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_features.h"

void choose_file(array * file, const char *dbpath, const struct taia *key)
{
	int len;
	array buf;
	byte_zero(&buf, sizeof(buf));

	len = fmt_time_str(&buf, key);
	array_cats(file, dbpath);
	array_cats0(file, buf.p);
}

#define CHOOSE_DB_FILE(f,p,e) \
	array (f); \
	byte_zero(&(f), sizeof((f))); \
	choose_file(&(f), (p), (&e->k)); \
	cdb_init_file((&f));
 // if not exists ..

void entry_dump(const struct nentry *e)
{
	static array b;

	fmt_time_hex(&b, &e->k);
	array_cat0(&b);
	sprintm("Dump: ", b.p, "->", e->e.p, "\n");
	print_time(&e->k);
	array_reset(&b);
}

int add_entry(const char *dbpath, struct nentry *entry)
{
	static array pk;	/*  packed key */

	fmt_time_bin(&pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);

	cdb_add(&dbfile, &pk, &entry->e);

	array_cat0(&entry->e);
	array_reset(&pk);
	return 1;
}

int add_entry_now(const char *dbpath, struct nentry *entry)
{
	taia_now(&entry->k);
	return add_entry(dbpath, entry);
}

int modify_entry(const char *dbpath, struct nentry *entry)
{
	static array pk;
	fmt_time_bin(&pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);

	cdb_mod(&dbfile, &pk, &entry->e);

//      entry_dump(entry);
	array_reset(&pk);
	return 1;
}

int delete_entry(const char *dbpath, struct nentry *entry)
{
	static array pk;

	fmt_time_bin(&pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	return cdb_del(&dbfile, &pk);
}

int show_entry(const char *dbpath, struct nentry *entry)
{
	struct cdb result;
	int err, fd, len;
	static array pac, dbfile;

	unsigned char buf[MAX_ENTRY_SIZE];

	fmt_time_bin(&pac, &entry->k);

	byte_zero(&dbfile, sizeof(dbfile));
	choose_file(&dbfile, dbpath, &entry->k);

	fd = open_read(dbfile.p);
	if (fd <= 0) {
		eprintmf("File not found", dbfile.p, "\n");
		return -1;
	}

	cdb_init(&result, fd);
	err = cdb_find(&result, (unsigned char *)pac.p, TAIA_PACK);

	static array hkey;
	fmt_time_hex(&hkey, &entry->k);

	if (err == 0) {
		eprintmf("Key ", hkey.p, "not found\n");
		return 0;
	} else if (err == -1) {
		eprintmf("Read error when trying to find key ", hkey.p, "\n");
		return -1;
	} else {
		len = cdb_datalen(&result);
		err = cdb_read(&result, buf, len, cdb_datapos(&result));
		if (err < 0) {
			eprintmf("Error reading entry ", hkey.p, "\n");
		}

		array_catb(&entry->e, (char *)buf, len);
		return 1;
	}
	array_reset(&dbfile);
	array_reset(&pac);
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
		strcpy(gerr.note, dbfile.p);
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

void dump_entries(array * entries)
{
	int i, len;
	struct nentry *tmp;
	char efmt[FMT_ULONG];

	len = array_length(entries, sizeof(struct nentry));
	efmt[fmt_int(efmt, len)] = 0;
	sprintmf("Found ", efmt, " entries\n");
	for (i = 0; i < len; i++) {
		tmp =
		    (struct nentry *)array_get(entries, sizeof(struct nentry),
					       i);
		entry_dump(tmp);
	}
}
