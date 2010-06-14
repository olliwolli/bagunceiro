#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <array.h>
#include <ctype.h>
#include <byte.h>
#include <caltime.h>
#include <errno.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_time.h"
#include "z_blog.h"
#include "z_features.h"
#include "format.h"

struct errors gerr = {
	.note = "",
	.error = 0,
	.type = N_NONE
};

void print_key(struct nentry *e)
{
	static array ts;
	fmt_time_hex(&ts, &e->k);

	array_cat0(&ts);
	sprint(" <span class=\"k\"><a href=\"" "?ts=");
	sprint(ts.p);
	sprint("\">link</a></span> ");

	sprint(" <span class=\"k\"><a href=\"" "?del=");
	sprint(ts.p);
	sprint("\">delete</a></span> ");

	sprint(" <span class=\"k\"><a href=\"" "?mod=");
	sprint(ts.p);
	sprint("\">modify</a></span> ");
	array_reset(&ts);
}

void print_value(struct nentry *e)
{
	sprint("<span class=\"c\">");
	sprint(e->e.p);
	sprint("</span>");
}

int load_config(blog_t * conf)
{
	return 0;
}

static void print_header(const blog_t * conf)
{
	/* set a cookie */
	switch (conf->csstype) {
	case CSS_SELECT:
		sprintm("Set-Cookie: css=", conf->css.p, "\n");
		break;
	case CSS_RESET:
		sprintm("Set-Cookie: css=\n");
		break;
	default:
		break;
	}

	sprintm("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");

	/* stylesheet */
	if (conf->csstype == CSS_SELECT || conf->csstype == CSS_COOKIE) {
		sprintm("<link rel=stylesheet type=\"text/css\" href=\"/",
			conf->css.p, "\" >\n");
	}

	/* rss version */
	sprintm("<link rel=\"alternate\" type=\"application/rss+xml\" title=\"",
		conf->title, " in rss\" href=\"", conf->host, "/rss.xml\">\n");

	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		sprintm("<script type=\"text/javascript\" src=\"/wysiwyg/wysiwyg.js\"></script>");
		sprintm("<link rel=\"stylesheet\" type=\"text/css\" href=\"/wysiwyg/wysiwyg.css\" />");
	}

	sprintm("<title>", conf->title, "</title>");
	sprintm("<div id=\"wrapper\"><div id=\"wrapper2\">");
	sprintm("<h1>",
		"<a href=\"", conf->script, "\">", conf->title, "</a></h1>\n");

}

static void print_footer(const blog_t * conf)
{
#ifdef ADMIN_MODE
	sprintm("<h1>",
		"<a href=\"", conf->script, "?add\">", "Add", "</a></h1>\n");
#endif
	sprintm("<h4>Please Note: ...  </span></h4>");
	sprintf("</body></html>\n");
}

static void print_notice(const blog_t * conf)
{
	switch (gerr.type) {
	case N_ERROR:
		sprint("\n<div id=\"error\">\n");
		sprintm("<div id=\"head\">Error: ", gerr.note, " ", "</div>");
		sprintm("<div id=\"body\">", strerror(gerr.error), "</div>");
		break;
	case N_NOTE:
		sprint("\n<div id=\"note\">\n");
		sprintm("<div id=\"head\">Note: ", gerr.note, "</div>");
		break;
	case N_ACTION:
		sprint("\n<div id=\"note\">\n");
		sprintm("<div id=\"head\">", gerr.note, "</div>");
		break;
	default:
		return;
	}
	sprint("</div>\n\n");
}

int print_body(array * blog, const blog_t * conf)
{
	int i, j, blen, elen;
	day_entry_t *de;
	struct nentry *e;
	char dfmt[20];
	int dlen;
	blen = array_length(blog, sizeof(struct day_entry));
	for (j = 0; j < blen; j++) {
		de = array_get(blog, sizeof(struct day_entry), j);
		elen = array_length(de->es, sizeof(struct nentry));
		dlen = caldate_fmtn(dfmt, de->date);	// TODO temporary here
		dfmt[dlen] = '\0';
		sprintm("<h3>", dfmt, "</h3>\n" " <ul>\n");
		/* loop through the hash table array to print out the data */
		for (i = 0; i < elen; i++) {
			e = array_get(de->es, sizeof(struct nentry), i);
			sprint("<li>");
			print_value(e);
			print_key(e);
			sprint("</li>\n");
		}
		sprint(" </ul>\n");
	}
	return 0;
}

int generate_entry_ts(const blog_t * conf, array * blog)
{
	array *day_items;
	struct day_entry *day = malloc(sizeof(struct day_entry));
	struct caltime *day_time = malloc(sizeof(struct caltime));
	struct nentry e;
	struct eops ops;

	byte_zero(&e, sizeof(e));

	ops.add_key = e_add_key;
	ops.add_val = e_add_val;
	ops.add_to_array = e_add_to_array;
	ops.alloc = e_malloc;

	scan_time_hex(&conf->qry.ts, &e.k);
	show_entry(conf->db, &e);
	caltime_utc(day_time, &e.k.sec, (int *)0, (int *)0);

	day_items = ops.alloc();
	ops.add_to_array(&e, day_items);
	day->es = day_items;
	day->date = &day_time->date;

	array_catb(blog, (char *)day, sizeof(struct day_entry));

	return 0;
}

int generate_entries_days(const blog_t * conf, array * blog)
{
	int err, start, stop;
	struct day_entry *day;
	array *day_items;
	struct caltime *day_time;
	struct taia tday;

	start = conf->qry.start;
	stop = conf->qry.end;
	taia_now(&tday);

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

static int print_add_entry(const blog_t * conf)
{
	print_header(conf);

	/* set appropriate notice */
	gerr.type = N_ACTION;
	strcpy(gerr.note, "Add a new entry");
	print_notice(conf);

	sprintm("<ul>\n");

	sprintm("<form  method=\"post\" action=\"", conf->script, "\">");
	sprint("<input type=\"hidden\" name=\"action\" value=\"add\">,");

	sprintm("<textarea class=\"wysiwyg\" name=\"input\" cols=\"100\" rows=\"30\">", "", "</textarea>");

	sprintm("<p><input type=\"submit\" value=\"Add\"></p>");
	sprintf("</form>");
	sprint(" </ul>\n");

	print_footer(conf);
	return 0;
}

static int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	print_header(conf);

	/* set appropriate notice */
	gerr.type = N_ACTION;
	strcpy(gerr.note, "Modify a entry");
	print_notice(conf);

	show_entry(conf->db, n);

	sprintm("<ul>\n");

	sprintm("<form  method=\"post\" action=\"", conf->script, "\">\n");
	sprintm("<input type=\"hidden\" name=\"action\" value=\"mod\">\n"
		"<input type=\"hidden\" name=\"key\" value=\"");
	print_key_plain(n);
	sprint("\">\n");

	sprint("<textarea class=\"wysiwyg\" name=\"input\" cols=\"80\" rows=\"10\">");
	if (n->e.p)
		sprint(n->e.p);
	else
		sprint("Entry not found");
	sprint("</textarea>");

	sprintm("<p><input type=\"submit\" value=\"Modify\"></p>");
	sprintf("</form>");
	sprint(" </ul>\n");

	print_footer(conf);
	return 0;
}

int print_template(array * blog, const blog_t * conf)
{
	/* TODO jsut for testing,can be called directyl afterwardss */
      print_do(blog, conf);
//
//	print_header(conf);
//	print_notice(conf);
//	print_body(blog, conf);
//	print_footer(conf);

	return 0;
}

static int print_query(const blog_t * conf)
{
	array blog;
	struct nentry n;
	int err;

	byte_zero(&n.e, sizeof(array));
	byte_zero(&blog, sizeof(blog));

	switch (conf->qry.action) {
	case QA_SHOW:
		switch (conf->qry.type) {
		case QRY_WEEK:
			err = generate_entries_days(conf, &blog);
			break;
		case QRY_TS:
			generate_entry_ts(conf, &blog);
			break;
		}
		print_template(&blog, conf);
		break;
	case QA_ADD:
		/* show the add dialog */
		if (!array_bytes(&conf->input)) {
			print_add_entry(conf);
			/* add a new entry */
		} else {
			gerr.type = N_NOTE;
			strcpy(gerr.note, "Entry added");
			array_cats0(&n.e, conf->input.p);
			add_entry_now(conf->db, &n);
			err = generate_entries_days(conf, &blog);
			print_template(&blog, conf);
		}
		break;
	case QA_DELETE:
		scan_time_hex(&conf->qry.ts, &n.k);
		err = delete_entry(conf->db, &n);
		if (err < 0) {
			gerr.type = N_ERROR;
			gerr.error = errno;
			strcpy(gerr.note, conf->qry.ts.p);
		} else {
			gerr.type = N_NOTE;
			strcpy(gerr.note, "Entry deleted");

		}
		generate_entries_days(conf, &blog);
		print_template(&blog, conf);
		break;
	case QA_MODIFY:
		scan_time_hex(&conf->qry.ts, &n.k);
		if (!array_bytes(&conf->input)) {
			print_mod_entry(conf, &n);
			/* add a new entry */
		} else {
			gerr.type = N_NOTE;
			strcpy(gerr.note, "Entry modified");
			array_cats0(&n.e, conf->input.p);
			modify_entry(conf->db, &n);

			err = generate_entries_days(conf, &blog);
			print_template(&blog, conf);
		}
		break;
	}

	/* TODO free day, day_items, blog */
	array_reset(&blog);
	array_reset(&n.e);
	return 0;
}

int print_blog(blog_t * conf)
{
	switch (conf->stype) {
	case S_HTML:
		conf->fmt = &fmt_html;
		break;
	case S_RSS:
		conf->fmt = &fmt_rss;
		break;
	default:
		sprint("Status: 404 Bad Request\r\n\r\n");
		return 1;
	}
	return print_query(conf);
}
