#include <stdlib.h>

#include <str.h>
#include <taia.h>

#include "z_features.h"
#include "z_cdb.h"
#include "z_time.h"
#include "z_entry.h"
#include "z_day_entry.h"

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
