#include <alloca.h>
#include <string.h>

#include "z_blog.h"
#include "z_rss.h"
#include "z_html5.h"
#include "z_http.h"
#include "z_day.h"
#include "z_fmtrss.h"
#include "z_format.h"

/* RSS */
/* I don't need this feature,
 * look at http_last_modified() on how to implement this */
static void fmt_date_rss(const struct nentry *n, char *d)
{
}

static inline int print_header_rss(const blog_t * conf)
{
	rss_http_header();
	rss_print_preface(conf->title, PROTO_HTTP, conf->host, conf->title,
		PROGRAM_NAME);
	return 0;
}

static void day_entries_rss(const blog_t * conf, struct day *de, size_t elen)
{
	int i;
	struct nentry *e;

	char lnk[FMT_PERMA_STATIC + strlen(conf->host)];
	char *cnt;
	char d[40] = "";

	lnk[0] = '\0';

	for (i = 0; i < elen; i++) {
		e = day_get_nentry(de, i);
		cnt = alloca(array_bytes(&e->e));	// FIXME

		fmt_perma_link(conf, e, lnk);
		html_remove_tags(e->e.p, -1, cnt);

		fmt_date_rss(e, d);
		rss_item(cnt, lnk, e->e.p, d);
	}
}

static void print_footer_rss(const blog_t * conf)
{
	rss_close_rss();
}

static void print_noentries_rss(const blog_t * conf)
{
}

/* STRUCTS */
struct fmting fmt__rss = {
	.day_entries = day_entries_rss,
	.header = print_header_rss,
	.footer = print_footer_rss,
	.noentries = print_noentries_rss
};
