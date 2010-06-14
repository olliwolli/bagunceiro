/*
 *  format.c
 *
 *  Oliver-Tobias Ripka (otr), otr@bockcay.de, 14.06.2010,
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

/* GENERAL */
#include <array.h>
#include "z_blog.h"
#include "format.h"
#include "z_entry.h"
#include "z_features.h"
#include "z_time.h"

/* print routines, mostly for showing entries */
void print_key_plain(struct nentry *e)
{
	static array ts;
	fmt_time_hex(&ts, &e->k);
	array_cat0(&ts);
	sprint(ts.p);
	array_reset(&ts);
}

void print_perma_link(const blog_t * conf, struct nentry *e)
{
	// TODO test
	sprintm(conf->script, "?ts=");
	print_key_plain(e);
}

void print_do(array * blog, const blog_t * conf)
{
	size_t blen, elen;
	day_entry_t *de;
	int i;

	blen = array_length(blog, sizeof(struct day_entry));

	if (conf->fmt->header)
		conf->fmt->header(conf);

	for (i = 0; i < blen; i++) {
		de = array_get(blog, sizeof(struct day_entry), i);
		elen = array_length(de->es, sizeof(struct nentry));
		conf->fmt->day_entries(conf, de, elen);
	}

	if (conf->fmt->footer)
		conf->fmt->footer(conf);
}

/* HTML */
void print_key_html(struct nentry *e)
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

void print_date_html(struct day_entry *e)
{
	char dfmt[20];
	size_t dlen;
	// TODO temporary here
	dlen = caldate_fmtn(dfmt, e->date);
	dfmt[dlen] = '\0';
	sprintm("<h3>", dfmt, "</h3>\n" " <ul>\n");
}

void print_notice_html(const blog_t * conf)
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

void print_header_html(const blog_t * conf)
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
	sprint("Content-type: text/html; charset=UTF-8\r\n\r\n");

	sprintm("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");

	print_notice_html(conf);
	/* stylesheet */
	if (conf->csstype == CSS_SELECT || conf->csstype == CSS_COOKIE) {
		sprintm("<link rel=stylesheet type=\"text/css\" href=\"/",
			conf->css.p, "\" >\n");
	}

	/* rss version */
	sprintm("<link rel=\"alternate\" type=\"application/rss+xml\" title=\"",
		conf->title, " in rss\" href=\"", conf->host, "/rss.xml\">\n");

	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		// TODO !!11
		sprintm("<script type=\"text/javascript\"",
			"src=\"/wysiwyg/wysiwyg.js\"></script>");
		sprintm("<link rel=\"stylesheet\" type=\"text/css\"",
			"href=\"/wysiwyg/wysiwyg.css\" />");
	}

	sprintm("<title>", conf->title, "</title>");
	sprintm("<div id=\"wrapper\"><div id=\"wrapper2\">");
	sprintm("<h1>",
		"<a href=\"", conf->script, "\">", conf->title, "</a></h1>\n");

}

void day_entries_html(const blog_t * conf, struct day_entry *de, size_t elen)
{
	int i;
	struct nentry *e;

	print_date_html(de);
	for (i = 0; i < elen; i++) {
		e = array_get(de->es, sizeof(struct nentry), i);
		sprint("<li>");

		sprint("<span class=\"c\">");
		sprint(e->e.p);
		sprint("</span>");

		print_key_html(e);
		sprint("</li>\n");
	}
	sprint(" </ul>\n");
}

void print_footer_html(const blog_t * conf)
{

#ifdef ADMIN_MODE
	sprintm("<h1>",
		"<a href=\"", conf->script, "?add\">", "Add", "</a></h1>\n");
#endif
	sprintm("<h4>Please Note: ...  </span></h4>");
	sprintf("</body></html>\n");

}

/* RSS */

void print_date_rss(struct day_entry *de)
{
	//TODO
}

void print_header_rss(const blog_t * conf)
{
	sprint("Content-type: application/rss+xml; charset=UTF-8\r\n" "\r\n");
	sprintm("<?xml version=\"1.0\"?>",
		"<rss version=\"2.0\">",
		"<channel>",
		"<title>",
		conf->title,
		"</title>",
		"<link>",
		conf->script,
		"</link>",
		"<description>",
		conf->title,
		"</description>"
		"<docs>http://blogs.law.harvard.edu/tech/rss</docs>"
		"<generator>", PROGRAM_NAME, "</generator>");

}

void day_entries_rss(const blog_t * conf, struct day_entry *de, size_t elen)
{
	int i;
	struct nentry *e;

	for (i = 0; i < elen; i++) {
		e = array_get(de->es, sizeof(struct nentry), i);
		sprintm("<item>", "<title>");
		sprint(e->e.p);
		sprintm("</title>", "<link>");
		print_perma_link(conf, e);
		sprintm("</link>", "<description><![CDATA[");
		sprint(e->e.p);
		sprintm("]]></description>", "<pubDate>");
		print_date_rss(de);
		sprintm("</pubDate>", "<guid>");
		print_key_plain(e);
		sprintm("</guid>", "</item>");
	}
}

void print_footer_rss(const blog_t * conf)
{
	sprintm("</channel></rss>");
}
/* STRUCTS */
struct fmting fmt_html = {
	.day_entries = day_entries_html,
	.header = print_header_html,
	.footer = print_footer_html
};

struct fmting fmt_rss = {
	.day_entries = day_entries_rss,
	.header = print_header_rss,
	.footer = print_footer_rss
};
