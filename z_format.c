#include <array.h>
#include <str.h>
#include "z_blog.h"
#include "z_format.h"
#include "z_entry.h"
#include "z_features.h"
#include "z_time.h"

/* Formatting of the output */

/* GENERAL */
static void print_key_plain(struct nentry *e)
{
	char buf[MAX_FMT_LENGTH_KEY];
	fmt_time_hex(buf, &e->k);
	sprint(buf);
}

static void print_perma_link(const blog_t * conf, struct nentry *e)
{
	// TODO test
	sprintm(conf->script, "?ts=");
	print_key_plain(e);
}

/* HTML */
static void print_key_html(struct nentry *e)
{
	char buf[MAX_FMT_LENGTH_KEY];
	fmt_time_hex(buf, &e->k);

	sprint(" <span class=\"k\"><a href=\"" "?ts=");
	sprint(buf);
	sprint("\">link</a></span> ");
#ifdef ADMIN_MODE
	sprint(" <span class=\"k\"><a href=\"" "?del=");
	sprint(buf);
	sprint("\">delete</a></span> ");

	sprint(" <span class=\"k\"><a href=\"" "?mod=");
	sprint(buf);
	sprint("\">modify</a></span> ");
#endif
}

static void print_date_html(struct day_entry *e)
{
	char dfmt[20];
	size_t dlen;
	// TODO temporary here
	dlen = caldate_fmtn(dfmt, e->date);
	dfmt[dlen] = '\0';
	sprintm("<h3>", dfmt, "</h3>\n" " <ul>\n");
}

static void print_notice_html(const blog_t * conf)
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
		sprintm("Set-Cookie: css=", conf->css, "\n");
		break;
	case CSS_RESET:
		sprintm("Set-Cookie: css=\n");
		break;
	default:
		break;
	}
	sprint("Content-type: text/html; charset=UTF-8\r\n\r\n");

	sprintm("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");

	/* stylesheet */
	if (conf->csstype == CSS_SELECT || conf->csstype == CSS_COOKIE) {
		sprintm("<link rel=stylesheet type=\"text/css\" href=\"/",
			conf->css, "\" >\n");
	}

	/* rss version */
	sprintm("<link rel=\"alternate\" type=\"application/rss+xml\" title=\"",
		conf->title, " in rss\" href=\"", conf->host, "/rss.xml\">\n");

#ifdef ADMIN_MODE
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		// TODO !!11
		sprintm("<script type=\"text/javascript\"",
			"src=\"/wysiwyg/wysiwyg.js\"></script>");
		sprintm("<link rel=\"stylesheet\" type=\"text/css\"",
			"href=\"/wysiwyg/wysiwyg.css\" />");
	}
#endif

	sprintm("<title>", conf->title, "</title>");
	sprintm("<div id=\"wrapper\"><div id=\"wrapper2\">");
	sprintm("<h1>",
		"<a href=\"", conf->script, "\">", conf->title, "</a></h1>\n");

	print_notice_html(conf);
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

#ifdef ADMIN_MODE
/*  HTML MODE ONLY */
int print_add_entry(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	gerr.type = N_ACTION;
	str_copy(gerr.note, "Add a new entry");
	print_notice_html(conf);

	sprintm("<ul>\n");

	sprintm("<form  method=\"post\" action=\"", conf->script, "\">");
	sprint("<input type=\"hidden\" name=\"action\" value=\"add\">,");

	sprintm("<textarea class=\"wysiwyg\" name=\"input\" cols=\"100\" rows=\"30\">", "", "</textarea>");

	sprintm("<p><input type=\"submit\" value=\"Add\"></p>");
	sprintf("</form>");
	sprint(" </ul>\n");

	print_footer_html(conf);
	return 0;
}

int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	print_header_html(conf);

	/* set appropriate notice */
	gerr.type = N_ACTION;
	str_copy(gerr.note, "Modify a entry");
	print_notice_html(conf);

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

	print_footer_html(conf);
	return 0;
}
#endif
/* RSS */
static void print_date_rss(struct day_entry *de)
{
	//TODO
}

void print_header_rss(const blog_t * conf)
{
	sprint("Content-type: application/rss+xml; charset=UTF-8\r\n" "\r\n");
	sprintm("<?xml version=\"1.0\"?>\n",
		"<rss version=\"2.0\">\n",
		"<channel>\n",
		"<title>",
		conf->title,
		"</title>\n",
		"<link>",
		conf->script,
		"</link>\n",
		"<description>",
		conf->title,
		"</description>\n"
		"<docs>http://blogs.law.harvard.edu/tech/rss</docs>\n"
		"<generator>", PROGRAM_NAME, "</generator>\n\n");

}

void day_entries_rss(const blog_t * conf, struct day_entry *de, size_t elen)
{
	int i;
	struct nentry *e;

	for (i = 0; i < elen; i++) {
		e = array_get(de->es, sizeof(struct nentry), i);
		sprintm("<item>\n", "<title>\n");
		sprint(e->e.p);
		sprintm("</title>\n", "<link>");
		print_perma_link(conf, e);
		sprintm("</link>\n", "<description><![CDATA[");
		sprint(e->e.p);
		sprintm("]]></description>\n", "<pubDate>");
		print_date_rss(de);
		sprintm("</pubDate>\n", "<guid>");
		print_key_plain(e);
		sprintm("</guid>\n", "</item>\n\n");
	}
}

void print_footer_rss(const blog_t * conf)
{
	sprintm("\n</channel></rss>");
}

/* GENERIC PRESENTATION */

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

void print_show(array * blog, blog_t * conf)
{
	size_t blen, elen;
	day_entry_t *de;
	int i;

	switch (conf->stype) {
	case S_HTML:
		conf->fmt = &fmt_html;
		break;
	case S_RSS:
		conf->fmt = &fmt_rss;
		break;
	default:
		sprint("Status: 404 Bad Request\r\n\r\n");
		return;
	}

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
