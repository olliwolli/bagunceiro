#include <string.h>
#include <stdlib.h>

#include <taia.h>
#include <textcode.h>

#include "z_blog.h"
#include "z_format.h"
#include "z_conf.h"
#include "z_http.h"
#include "z_fmtrss.h"
#include "z_fmthtml.h"

#ifdef ADMIN_MODE_PASS
#include <stdlib.h>
#endif

/* Formatting of the output */
/* needs FMT_TAIA_HEX */
void fmt_key_plain(struct nentry *e, char *fmt)
{
	fmt[fmt_hexdump(fmt, e->k.p, TAIA_PACK)] = 0;
	reduce_ts(fmt);
}

/* needs at most FMT_PERMA_STATIC + strlen(conf->host) characters */

int fmt_perma_link(const blog_t * conf, struct nentry *e, char *fmt)
{
	char k[FMT_TAIA_HEX];
	int ls = strlen(conf->host);

	if (ls >= 256 - FMT_PERMA_STATIC)
		return 0;

	if (fmt == NULL)
		return ls + FMT_PERMA_STATIC;

	fmt[0] = 0;

	fmt_key_plain(e, k);

	strcat(fmt, PROTO_HTTP);
	strcat(fmt, conf->host);
	strcat(fmt, "?" QUERY_TS "=");
	strcat(fmt, k);
	return FMT_PERMA_STATIC + ls;
}

int print_show(struct result *res, blog_t * conf)
{
	int err;
	size_t blen, elen;
	struct day *de;
	int i;

#if defined(WANT_HTTP_304) && !defined(WANT_FAST_CGI)
	time_t t;

	/* may exit */
	if (conf->mod) {
		if ((t = http_if_modified_since(conf->mod)) != 0)
			http_last_modified(t);
		else {
			http_not_changed_modified();

			return 304;
		}
	}
#endif

	switch (conf->qry.stype) {
	case S_HTML:
		conf->fmt = &fmt__html;
		break;
	case S_RSS:
		conf->fmt = &fmt__rss;
		break;
	default:
		http_not_found();
		return 404;
	}

	blen = result_length(res);

	if (conf->fmt->header) {
		err = conf->fmt->header(conf);
		if (err)
			return 1;
	}

	for (i = 0; i < blen; i++) {
		de = result_get_day_entry(res, i);
		elen = day_length(de);
		conf->fmt->day_entries(conf, de, elen);
	}

	if (i == 0)
		conf->fmt->noentries(conf);

	if (conf->fmt->footer)
		conf->fmt->footer(conf);

	return 0;
}
