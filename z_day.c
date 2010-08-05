#include <stdlib.h>
#include <string.h>

#include <str.h>
#include <taia.h>

#include "z_features.h"
#include "z_cdbb.h"
#include "z_time.h"
#include "z_entry.h"
#include "z_day.h"

void day_init(struct day *d)
{
	memset(&d->es, 0, sizeof(array));
}

struct day *day_new()
{
	struct day *d;
	d = malloc(sizeof(struct day));
	day_init(d);
	return d;
}

void day_free(struct day *d)
{
	int i, len;
	struct nentry *n;
	len = day_length(d);
	if (&d->es != NULL) {
		for (i = 0; i < len; i++) {
			n = day_get_nentry(d, i);
			free_nentry(n);
		}
		array_reset(&d->es);
	}
	free(d);
}

size_t day_length(struct day *d)
{
	return array_length(&d->es, sizeof(struct nentry **));
}

struct nentry *day_get_nentry(struct day *d, int i)
{
	return *(struct nentry **)
		array_get(&d->es, sizeof(struct nentry **), i);
}

void day_add_nentry(struct day *d, struct nentry *n)
{
	array_catb(&d->es, (char *)&n, sizeof(struct nentry **));
}

int cdbb_fetch_day(struct cdbb *a, struct day *entries, const struct taia *day)
{
	struct nentry *dentry;
	struct nentry *entry;
	char *key;
	int err, len, i;
	char buf[FMT_TAIA_STR + 1] = "";

	len = fmt_time_str(buf, day);
	buf[len] = '@';		/* fits, but is not null terminated */
	dentry = new_nentry();

	/* lookup all entries for that day */
	err = cdbb_read_nentry(a, buf, len + 1, dentry);

	if (err <= 0)
		goto err;

	/* reverse order since recent blog entires are added to the end */
	for (i = array_length(&dentry->e, TAIA_PACK) - 1; i >= 0; i--) {
		key = array_get(&dentry->e, TAIA_PACK, i);
		entry = new_nentry();

		/* ALLOCATION */
		err = cdbb_read_nentry(a, key, TAIA_PACK, entry);
		if (err < 0)
			free_nentry(entry);
		else
			day_add_nentry(entries, entry);
	}

err:
	/* free hashes */
	free_nentry(dentry);

	return err;
}
