#include <array.h>
#include <str.h>
#include <textcode.h>
#include "z_blog.h"
#include "z_format.h"
#include "z_entry.h"
#include "z_features.h"
#include "z_time.h"

#ifdef ADMIN_MODE_PASS
#include <stdlib.h>
#endif

/* Formatting of the output */

/* GENERAL */
static void print_key_plain(struct nentry *e)
{
	char buf[FMT_TAIA_HEX];
	fmt_time_hex(buf, &e->k);
	reduce_ts(buf);
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
	char buf[FMT_TAIA_HEX];
	fmt_time_hex(buf, &e->k);
	reduce_ts(buf);

	sprintm(" <span class=\"k\"><a href=\"" "?ts=",
		buf, "\">link</a></span> ");
#ifdef ADMIN_MODE
	sprintm(" <span class=\"k\"><a href=\"" "?del=",
		buf, "\">delete</a></span> ");

	sprintm(" <span class=\"k\"><a href=\"" "?mod=",
		buf, "\">modify</a></span> ");
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

#ifdef WANT_ERROR_PRINT
static void print_notice_html(const blog_t * conf)
{
	switch (gerr.type) {
	case N_ERROR:
		sprintm("\n<div id=\"error\">\n",
			"<div id=\"head\">Error: ", gerr.note, " ", "</div>",
//              "<div id=\"body\">", strerror(gerr.error), "</div>",
			"<div id=\"body\">", "Sys error", "</div>");
		break;
	case N_NOTE:
		sprintm("\n<div id=\"note\">\n",
			"<div id=\"head\">Note: ", gerr.note, "</div>");
		break;
	case N_ACTION:
		sprintm("\n<div id=\"note\">\n",
			"<div id=\"head\">", gerr.note, "</div>");
		break;
	default:
		return;
	}
	sprint("</div>\n\n");
}
#else
static void print_notice_html(const blog_t * conf)
{
}
#endif

#define CONTENT_TYPE_HTML "Content-type: text/html; charset=UTF-8\r\n\r\n"
#define DOCTYPE "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"

void print_header_html(const blog_t * conf)
{
	if (conf->csstype == CSS_SELECT) {
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
	}

#ifdef ADMIN_MODE_PASS
	if (conf->authpost) {
		char hexdump[SHA256_DIGEST_LENGTH*2+1];

		if (conf->csstype == CSS_SELECT)
			sprint("; ");
		else if (conf->authpost)
			sprint("Set-Cookie: ");

		sprint("token=");
		fmt_hexdump(hexdump, (const char * )conf->token, SHA256_DIGEST_LENGTH);
		sprintn((char *)hexdump, SHA256_DIGEST_LENGTH*2);
		sprint("\n");
	}
#endif
	sprintm(CONTENT_TYPE_HTML, DOCTYPE);

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
		sprintm("<script type=\"text/javascript\""
			"src=\"/wysiwyg/wysiwyg.js\"></script>"
			"<link rel=\"stylesheet\" type=\"text/css\""
			"href=\"/wysiwyg/wysiwyg.css\" />");
	}
#endif

	sprintm("<title>", conf->title, "</title>"
		"<div id=\"wrapper\"><div id=\"wrapper2\">"
		"<h1>",
		"<a href=\"", conf->script, "\">", conf->title, "</a></h1>\n");

#ifdef ADMIN_MODE_PASS
	if( !conf->ssl ){
		sprintf("Need ssl connetion");
		exit(1);
	}

	if (conf->authcache) {
		sprint("Authenticated\n");
	}
#endif

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

		sprintm("<span class=\"c\">", e->e.p, "</span>");

		print_key_html(e);
		sprint("</li>\n");
	}
	sprint(" </ul>\n");
}

void print_footer_html(const blog_t * conf)
{

#ifdef ADMIN_MODE
	sprintm("<h1>"
		"<a href=\"", conf->script, "?add\">" "Add" "</a></h1>\n");
#endif
	sprintmf("<h4>Please Note: ...  </span></h4>" "</body></html>\n");

}

#ifdef ADMIN_MODE
/*  HTML MODE ONLY */
int print_add_entry(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Add a new entry", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<ul>\n"
		"<form  method=\"post\" action=\"", conf->script, "\">"
		"<input type=\"hidden\" name=\"action\" value=\"add\">,"
		"<textarea class=\"wysiwyg\" name=\"input\" cols=\"100\" rows=\"30\">",
		"",
		"</textarea>" "<p><input type=\"submit\" value=\"Add\"></p>"
		"</form>" " </ul>\n");

	print_footer_html(conf);
	return 0;
}

int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Modify an entry", 0, N_ACTION);
	print_notice_html(conf);

	show_entry(conf->db, n);

	sprintm("<ul>\n"
		"<form  method=\"post\" action=\"", conf->script, "\">\n",
		"<input type=\"hidden\" name=\"action\" value=\"mod\">\n"
		"<input type=\"hidden\" name=\"key\" value=\"");

	print_key_plain(n);
	sprint("\">\n");

	sprint("<textarea class=\"wysiwyg\" name=\"input\" cols=\"80\" rows=\"10\">");
	if (n->e.p)
		sprint(n->e.p);
	else
		sprint("Entry not found");
	sprint("</textarea>");

	sprintm("<p><input type=\"submit\" value=\"Modify\"></p>"
		"</form>" " </ul>\n");

	print_footer_html(conf);
	return 0;
}
#endif
#ifdef ADMIN_MODE_PASS
void print_login(const blog_t * conf)
{
	print_header_html(conf);

	set_err("Login", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<ul>\n"
		"<form  method=\"post\" action=\"", conf->script, "\">\n",
		"<input name=\"log\" type=\"password\" size=\"12\" maxlength=\"12\">\n");

	sprintm("<p><input type=\"submit\" value=\"Login\"></p>"
		"</form>" " </ul>\n");

	print_footer_html(conf);
}

#endif
/* RSS */
static void print_date_rss(struct day_entry *de)
{
	//TODO
}

#define CONTENT_TYPE_RSS "Content-type: application/rss+xml; charset=UTF-8\r\n" "\r\n"
void print_header_rss(const blog_t * conf)
{
	sprintm(CONTENT_TYPE_RSS
		"<?xml version=\"1.0\"?>\n"
		"<rss version=\"2.0\">\n"
		"<channel>\n"
		"<title>",
		conf->title,
		"</title>\n"
		"<link>",
		conf->script,
		"</link>\n"
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
		sprintm("<item>\n" "<title>\n", e->e.p, "</title>\n" "<link>");
		print_perma_link(conf, e);
		sprintm("</link>\n" "<description><![CDATA[",
			e->e.p, "]]></description>\n" "<pubDate>");
		print_date_rss(de);
		sprintm("</pubDate>\n" "<guid>");
		print_key_plain(e);
		sprintm("</guid>\n" "</item>\n\n");
	}
}

void print_footer_rss(const blog_t * conf)
{
	sprint("\n</channel></rss>");
}

/* GENERIC PRESENTATION */

/* STRUCTS */
struct fmting fmt__html = {
	.day_entries = day_entries_html,
	.header = print_header_html,
	.footer = print_footer_html
};

struct fmting fmt__rss = {
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
		conf->fmt = &fmt__html;
		break;
	case S_RSS:
		conf->fmt = &fmt__rss;
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
