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

#define CONTENT_TYPE_HTML "Content-type: text/html; charset=UTF-8\r\n\r\n"
#define DOCTYPE "<!doctype html>\n"

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

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
void print_tiny_html_editor()
{
	sprintm(
	"<script type=\"text/javascript\">"
	"new TINY.editor.edit('editor',{"
	"	id:'input',"
	"	width:800,"
	"	height:175,"
	"	cssclass:'te',"
	"	controlclass:'tecontrol',"
	"	rowclass:'teheader',"
	"	dividerclass:'tedivider',"
	"	controls:['bold','italic','underline','strikethrough','|','subscript','superscript','|',"
	"			  'orderedlist','unorderedlist','|','outdent','indent','|','leftalign',"
	"			  'centeralign','rightalign','blockjustify','|','unformat','|','undo','redo','n',"
	"			  'font','size','style','|','image','hr','link','unlink','|','cut','copy','paste','print'],"
	"	footer:true,"
	"	fonts:['Verdana','Arial','Georgia','Trebuchet MS'],"
	"	xhtml:true,"
	"	cssfile:'style.css',"
	"	bodyid:'editor',"
	"	footerclass:'tefooter',"
	"	toggle:{text:'source',activetext:'wysiwyg',cssclass:'toggle'},"
	"	resize:{cssclass:'resize'}"
	"});"
	"</script>"
	);
}
#else
void print_tiny_html_editor(){}
#endif

static void print_date_html(struct day_entry *e)
{
	char dfmt[20];
	size_t dlen;
	// TODO temporary here
	dlen = caldate_fmtn(dfmt, &e->time.date);
	dfmt[dlen] = '\0';
	sprintm("<h3>", dfmt, "</h3>\n\n");
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

static void print_header_html(const blog_t * conf)
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

#ifdef ADMIN_MODE_PASS
	if (conf->qry.action == QA_LOGOUT) {
		sprint("Set-Cookie: token=\n");
	}

	if (conf->authpost) {
		char hexdump[SHA256_DIGEST_LENGTH * 2 + 1];

		if (conf->csstype == CSS_SELECT)
			sprint("; ");
		else if (conf->authpost)
			sprint("Set-Cookie: ");

		sprint("token=");
		fmt_hexdump(hexdump, (const char *)conf->token,
			SHA256_DIGEST_LENGTH);
		sprintn((char *)hexdump, SHA256_DIGEST_LENGTH * 2);
		/* make it a little more secure */
		sprintm("; Secure; HttpOnly; Discard; Max-Age=300");
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

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		sprintm("<link rel=\"stylesheet\" href=\""
			TINY_HTML_PATH
			"style.css\" />\n"
			"<script type=\"text/javascript\" src=\""
			TINY_HTML_PATH
			"packed.js\"></script>\n\n"
		);
	}
#endif

	sprintm("<title>", conf->title, "</title>\n"
		"<div id=\"wrapper\"><div id=\"wrapper2\">\n"
		"<h1><a href=\"", conf->script, "\">", conf->title, "</a></h1>\n\n");

#ifdef ADMIN_MODE_PASS
	if (!conf->ssl) {
		sprintf("Need ssl connetion");
		exit(1);
	}
#endif

	print_notice_html(conf);
}

static void day_entries_html(const blog_t * conf, struct day_entry *de, size_t elen)
{
	int i;
	struct nentry *e;

	print_date_html(de);
	sprint("<ul>\n");
	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
		sprint(" <li>");

		sprintm("<span class=\"c\">", e->e.p, "</span>");

		print_key_html(e);
		sprint("</li>\n");
	}
	sprint("</ul>\n");
}

static void print_footer_html(const blog_t * conf)
{

#ifdef ADMIN_MODE
	sprintm("<h4>"
		"<a href=\"", conf->script, "?add\">" "Add entry | " "</a>\n");
#endif
#ifdef ADMIN_MODE_PASS
	sprintm("<a href=\"", conf->script, "?");
	if (conf->authcache) {
		sprint("logout\">" "Logout");
	} else {
		sprint("login\">" "Login");
	}
	sprint("</a></h4>\n");

#endif
#ifdef WANT_MONTH_BROWSING
	/* TODO optimize */
	/* print month selection */
	char older[FMT_CALDATE];
	char newer[FMT_CALDATE];
	struct taia now;
	struct caltime ct;

	if(str_equal(conf->qry.mon, "")){
		taia_now(&now);
		caltime_utc(&ct, &now.sec, (int *)0, (int *)0);
	}else{
		caldate_scan(conf->qry.mon , &ct.date);
	}

	ct.date.month--;
	caldate_normalize(&ct.date);
	older[caldate_fmt(older, &ct.date)] = 0;
	older[str_rchr(older, '-')] = 0;

	ct.date.month+= 2;
	caldate_normalize(&ct.date);
	newer[caldate_fmt(newer, &ct.date)] = 0;
	newer[str_rchr(newer, '-')] = 0;

	sprintm("\n\n<a href=\"?mn=", older, "\">older</a> -");
	if(conf->script)
		sprintm("<a href=\"", conf->script, "\">now</a>");

	sprintm("- <a href=\"?mn=", newer, "\">later</a>\n");
	sprintmf("<h4>Please Note: ...  </span></h4>" "</body></html>\n");
#endif
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
		"<form  onsubmit='editor.post();' method=\"post\" action=\"", conf->script, "\">\n"
		"<input type=\"hidden\" name=\"action\" value=\"add\">\n"
		"<textarea name=\"input\" id=\"input\" style=\"width:400px; height:200px\"></textarea>\n"
		"<p><input type=\"submit\" value=\"Add\"></p> \n"
		"</form>" "</ul>\n"
		);
	print_tiny_html_editor();

	print_footer_html(conf);
	return 0;
}

int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Modify an entry", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<ul>\n"
		"<form  onsubmit='editor.post();' method=\"post\" action=\"", conf->script, "\">\n",
		"<input type=\"hidden\" name=\"action\" value=\"mod\">\n"
		"<input type=\"hidden\" name=\"key\" value=\"");

	print_key_plain(n);
	sprint("\">\n");

	sprint("<textarea name=\"input\" id=\"input\" style=\"width:400px; height:200px\">");
	if (n->e.p)
		sprint(n->e.p);
	else
		sprint("Entry not found");
	sprint("</textarea>\n");

	print_tiny_html_editor();

	sprintm("<p><input type=\"submit\" value=\"Modify\"></p>"
		"</form>" "</ul>\n");

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
		"<input name=\"login\" type=\"password\" size=\"12\" maxlength=\"12\">\n");

	sprintm("<p><input type=\"submit\" value=\"Login\"></p>"
		"</form>" "</ul>\n");

	print_footer_html(conf);
}

#endif
/* RSS */
static void print_date_rss(struct day_entry *de)
{
	//TODO
}

#define CONTENT_TYPE_RSS "Content-type: application/rss+xml; charset=UTF-8\r\n" "\r\n"
static void print_header_rss(const blog_t * conf)
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

static void day_entries_rss(const blog_t * conf, struct day_entry *de, size_t elen)
{
	int i;
	struct nentry *e;

	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
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

static void print_footer_rss(const blog_t * conf)
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
		elen = array_length(&de->es, sizeof(struct nentry));
		conf->fmt->day_entries(conf, de, elen);
	}
	if( i == 0){
		sprint("No entries found");
	}

	if (conf->fmt->footer)
		conf->fmt->footer(conf);
}
