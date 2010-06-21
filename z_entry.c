#include <stdlib.h>

#include <open.h>
#include <cdb.h>
#include <str.h>
#include <taia.h>
#include <array.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_features.h"

inline static void choose_file(array * file, const char *dbpath,
	const struct taia *key)
{
	int len;
	char buf[FMT_TAIA_STR];

#ifdef WANT_COHERENT_TIME
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
	memset(&(f), 0, sizeof((f))); \
	choose_file(&(f), (a), (&e->k)); 

#ifdef ADMIN_MODE
int add_entry(const char *dbpath, struct nentry *entry)
{
	int err;

	char pk[TAIA_PACK];
	taia_pack(pk, &entry->k);

	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	err = cdb_add(dbfile.p, pk, TAIA_PACK, &entry->e);

	array_cat0(&entry->e);

	array_reset(&dbfile);
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
	array_reset(&dbfile);

	return err;
}

int delete_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pk[TAIA_PACK];

	taia_pack(pk, &entry->k);
	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	err = cdb_del(dbfile.p, pk, TAIA_PACK);
	array_reset(&dbfile);

	return err;
}
#endif
#ifdef ADMIN_MODE
void entry_dump(const struct nentry *e)
{

	char b[FMT_TAIA_HEX];
	fmt_time_hex(b, &e->k);
	sprintmf("Dump: ", b, "->", e->e.p, "\n");

}

void dump_entries(array * entries)
{
	int i, len;
	struct nentry *tmp;
	char efmt[FMT_ULONG];

	len = array_length(entries, sizeof(struct nentry));
	efmt[fmt_int(efmt, len)] = 0;
	sprintm("Found ", efmt, " entries\n");
	for (i = 0; i < len; i++) {
		tmp = (struct nentry *)array_get(entries, sizeof(struct nentry),
			i);
		entry_dump(tmp);
	}
}
#endif

int show_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pac[TAIA_PACK];

	CHOOSE_DB_FILE(dbfile, dbpath, entry);
	taia_pack(pac, &entry->k);
	err = cdb_get(dbfile.p, pac, TAIA_PACK, &entry->e);

	if (err == 0)
		eprintmf("Key not found\n");
	else if (err == -1)
		eprintmf("Read error when trying to find key\n");

	array_reset(&dbfile);

	return err;
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
	array_catb(arr, (char *)e, sizeof(struct nentry));
}

void *e_malloc()
{
	struct nentry *tmp;

	tmp = malloc(sizeof(struct nentry));
	memset(&tmp->e, 0, sizeof(array));

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

	memset(&dbfile, 0, sizeof(dbfile));
	memset(&key, 0, sizeof(key));
	choose_file(&dbfile, dbpath, day);
	err = cdb_read_all(dbfile.p, entries, &ops);
	if (err == -2) {
		set_err(dbfile.p, ESPIPE, N_ERROR);
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

int do_day_index(array * fmt, unsigned char * key, size_t ks)
{
	char buf[FMT_TAIA_STR];
	struct taia l;
	int len;

	taia_unpack( (char *)key, &l);

	len = fmt_time_str(buf, &l);

	array_catb(fmt,buf, len);
	array_cats(fmt, "@");

	return 0;
}

//#include <stdlib.h>
//int _show_day(struct cdb * result, array * entries, const struct taia * day)
//{
//	array * key;
//	int err,len,i;
//	char buf[FMT_TAIA_STR+1] = "";
//
//	len= fmt_time_str(buf, day);
//	strcat(buf, "@");
//
//	array hashes;
//	memset(&hashes, 0, sizeof(array));
//
//	/* lookup all entries for that day */
//	err = _cdb_get(result,buf,len,&hashes);
//
//	/*  iterate of hashes */
//	for(i = 0; i < array_length(&hashes, sizeof(array)); i++){
//		key = array_get(&hashes, sizeof(array), i);
//		_cdb_get(result, key->p, array_bytes(key), entries);
//	}
//
//	return err;
//
//}
