#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <array.h>
#include <ctype.h>
#include <byte.h>
#include <caltime.h>
#include <errno.h>
#include <str.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_blog.h"
#include "z_features.h"
#include "z_format.h"

/* Parses query and fetches data from database */
static int fetch_entry_ts(const blog_t * conf, array * blog)
{
	array *day_items;
	struct day_entry *day = malloc(sizeof(struct day_entry));
	struct caltime *day_time = malloc(sizeof(struct caltime));
	struct nentry e;

	byte_zero(&e, sizeof(e));

	scan_time_hex(conf->qry.ts, &e.k);
	show_entry(conf->db, &e);
	caltime_utc(day_time, &e.k.sec, (int *)0, (int *)0);

	day_items = e_malloc();
	e_add_to_array(&e, day_items);
	day->es = day_items;
	day->date = &day_time->date;

	array_catb(blog, (char *)day, sizeof(struct day_entry));

	return 0;
}

static int fetch_entries_days(const blog_t * conf, array * blog)
{
	int err, start, stop;
	struct day_entry *day;
	struct caltime *day_time;
	struct taia tday;
	array *day_items;

	start = conf->qry.start;
	stop = conf->qry.end;
	taia_now(&tday);

	err = 0;

	for (; start < stop; start++) {
		/* allocate memory */
		day = malloc(sizeof(struct day_entry));
		day_items = malloc(sizeof(struct nentry));
		day_time = malloc(sizeof(struct caltime));
		day_items->allocated = 0;
		day_items->initialized = 0;
		byte_zero(day_items, sizeof(struct nentry));
		//TODO check if file exists in order to avoid mallocs above
		/* get entries for calculated day */
		err = show_day(conf->db, day_items, &tday);

		if (err <= 0)	/* File not found or 0 entries */
			goto sub;

		caltime_utc(day_time, &tday.sec, (int *)0, (int *)0);

		/* fill in the entries and date */
		day->es = day_items;
		day->date = &day_time->date;
		/* add the day to the blog */
		array_catb(blog, (char *)day, sizeof(struct day_entry));
		if (array_failed(blog)) {
			eprintf("Could not allocate memory!");
			exit(-1);
		}
sub:
		/* calculate day */
		ht_sub_days(&tday, 1);
	}

	return err;
}

int handle_query(blog_t * conf)
{
	array blog;
	struct nentry n;
	int err;

	byte_zero(&n.e, sizeof(array));
	byte_zero(&blog, sizeof(array));

	switch (conf->qry.action) {
	case QA_SHOW:
		switch (conf->qry.type) {
		case QRY_WEEK:
			err = fetch_entries_days(conf, &blog);
			break;
		case QRY_TS:
			fetch_entry_ts(conf, &blog);
			break;
		}
		print_show(&blog, conf);
		break;

#ifdef ADMIN_MODE
	case QA_ADD:
		/* show the add dialog */
		if (!array_bytes(&conf->input)) {
			print_add_entry(conf);
			/* add a new entry */
		} else {
			set_err("Entry added", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			add_entry_now(conf->db, &n);
			err = fetch_entries_days(conf, &blog);
			print_show(&blog, conf);
		}
		break;
	case QA_DELETE:
		scan_time_hex(conf->qry.ts, &n.k);
		err = delete_entry(conf->db, &n);
		if (err < 0) {
			set_err(conf->qry.ts, errno, N_ERROR);
		} else {
			set_err("Entry deleted", 0, N_NOTE);
		}
		fetch_entries_days(conf, &blog);
		print_show(&blog, conf);
		break;
	case QA_MODIFY:
		scan_time_hex(conf->qry.ts, &n.k);
		if (!array_bytes(&conf->input)) {
			print_mod_entry(conf, &n);
			/* add a new entry */
		} else {
			set_err("Entry modified", 0, N_NOTE);
			array_cats0(&n.e, conf->input.p);
			modify_entry(conf->db, &n);

			err = fetch_entries_days(conf, &blog);
			print_show(&blog, conf);
		}
		break;
#endif
#ifdef ADMIN_MODE_PASS
	case QA_LOGIN:
		print_login(conf);
		break;
	case QA_LOGOUT:
		print_show(&blog, conf);
		break;

#endif
	}
	/* TODO free day, day_items, blog */
	array_reset(&blog);
	array_reset(&n.e);
	return 0;
}
