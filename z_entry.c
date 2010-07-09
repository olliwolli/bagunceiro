#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <open.h>
#include <cdb.h>
#include <str.h>
#include <taia.h>
#include <array.h>
#include <textcode.h>

#include "z_entry.h"
#include "z_time.h"
#include "z_features.h"

int fmt_day_idx(array * fmt, struct taia *l)
{
	char buf[FMT_TAIA_STR];
	int len;

	len = fmt_time_str(buf, l);

	array_catb(fmt, buf, len);
	array_cats(fmt, "@");

	return 0;
}

int show_entry(struct cdbb *a, struct taia *k, struct nentry *n)
{
	int err;
	char pac[TAIA_PACK];
	taia_pack(pac, k);
	err = cdbb_read_nentry(a, pac, TAIA_PACK, n);

	return err;
}

#ifdef ADMIN_MODE
void blog_modified(struct cdbb *a)
{
	char pk[TAIA_PACK];
	struct taia t;

	taia_now(&t);
	taia_pack(pk, &t);
	cdbb_rep(a, "!lastmodified", 13, pk,TAIA_PACK);
}

void add_entry(struct cdbb *a, struct taia *k, char *v, size_t vs)
{
	char pk[TAIA_PACK];
	array dayidx;

	memset(&dayidx, 0, sizeof(array));
	fmt_day_idx(&dayidx, k);
	taia_pack(pk, k);

	/* entry + idx*/
	cdbb_add(a, pk, TAIA_PACK, v, vs);
	cdbb_add(a, dayidx.p, array_bytes(&dayidx), pk, TAIA_PACK);

	blog_modified(a);
	array_reset(&dayidx);
}

void add_entry_now(struct cdbb *a, char *v, size_t vs)
{
	struct taia k;
	taia_now(&k);
	add_entry(a, &k, v, vs);
}

void modify_entry(struct cdbb *a, struct taia *k, char *v, size_t vs)
{
	char pk[TAIA_PACK];
	taia_pack(pk, k);
	cdbb_mod(a, pk, TAIA_PACK, v, vs);
	blog_modified(a);
}

void delete_entry(struct cdbb *a, struct taia *k)
{
	char pk[TAIA_PACK];
	array dayidx;

	memset(&dayidx, 0, sizeof(array));
	fmt_day_idx(&dayidx, k);

	taia_pack(pk, k);

	cdbb_del(a, pk, TAIA_PACK);
	cdbb_del_val(a, dayidx.p, array_bytes(&dayidx), pk, TAIA_PACK);
	blog_modified(a);
}
#endif
#ifdef ADMIN_MODE
void entry_dump(const struct nentry *e)
{
	char b[FMT_TAIA_HEX];
	b[fmt_hexdump(b, e->k.p, TAIA_PACK)]=0;

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

