#include <stdlib.h>
#include <ctype.h>

#include <open.h>
#include <cdb.h>
#include <str.h>
#include <taia.h>
#include <array.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_features.h"

struct nentry *new_nentry(void)
{
	struct nentry *n;
	n = malloc(sizeof(struct nentry));
	memset(&n->e, 0, sizeof(array));
	return n;
}

void free_nentry(struct nentry *n)
{
	array_reset(&n->e);
	free(n);
}

struct day_entry *new_day_entry()
{
	struct day_entry *d;
	d = malloc(sizeof(struct day_entry));
	memset(&d->es, 0, sizeof(array));
	return d;
}

void free_day_entry(struct day_entry *d)
{
	int i;
	struct nentry *n;
	if (&d->es != NULL) {
		for (i = 0; i < array_length(&d->es, sizeof(struct nentry));
			i++) {
			n = array_get(&d->es, sizeof(struct nentry), i);
			array_reset(&n->e);
		}
		array_reset(&d->es);
	}
}

//void trimright(char *s)
//{
//  char *end;
//
//  end = s + strlen(s) - 1;
//  while(end > s && isspace(*end)) end--;
//  *(end+1) = 0;
//}


#ifdef ADMIN_MODE
int add_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pk[TAIA_PACK];

	taia_pack(pk, &entry->k);
	err = cdb_add_idx(dbpath, pk, TAIA_PACK, entry->e.p,
		array_bytes(&entry->e));
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

	err = cdb_mod(dbpath, pk, TAIA_PACK, entry->e.p,
		array_bytes(&entry->e));

	return err;
}

int delete_entry(const char *dbpath, struct nentry *entry)
{
	int err;
	char pk[TAIA_PACK];

	taia_pack(pk, &entry->k);
	err = cdb_del_idx(dbpath, pk, TAIA_PACK);

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

int _show_entry(struct cdb *result, struct nentry *entry)
{
	int err;
	char pac[TAIA_PACK];

	taia_pack(pac, &entry->k);
	err = _cdb_get(result, pac, TAIA_PACK, entry);

	return err;
}

int _show_day(struct cdb *result, array * entries, const struct taia *day)
{
	struct nentry dentry;
	struct nentry *entry;
	char *key;
	int err, len, i;

	char buf[FMT_TAIA_STR + 1] = "";

	len = fmt_time_str(buf, day);
	buf[len] = '@';		/* len = len+1 */

	memset(&dentry, 0, sizeof(struct nentry));

	/* lookup all entries for that day */
	err = _cdb_get(result, buf, len + 1, &dentry);

	if (err <= 0)
		goto err;

	for (i = 0; i < array_length(&dentry.e, TAIA_PACK); i++) {
		key = array_get(&dentry.e, TAIA_PACK, i);
		/* ALLOCATION */
		entry = new_nentry();

		_cdb_get(result, key, TAIA_PACK, entry);
		/* add to array */
		array_catb(entries, (char *)entry, sizeof(struct nentry));

	}

err:
	/* free hashes */
	array_reset(&dentry.e);
	free(dentry.kp);

	return err;
}
