#include <array.h>
#include <str.h>
#include <alloca.h>
#include <textcode.h>
#include "z_blog.h"
#include "z_format.h"
#include "z_entry.h"
#include "z_day_entry.h"
#include "z_features.h"
#include "z_time.h"
#include "z_html5.h"
#include "z_http.h"
#include "z_rss.h"

#ifdef ADMIN_MODE_PASS
#include <stdlib.h>
#endif

/* Formatting of the output */

/* needs FMT_TAIA_HEX */
static void fmt_key_plain(struct nentry *e, char * fmt)
{
	fmt_time_hex(fmt, &e->k);
	reduce_ts(fmt);
}

/* max 256 */
static void fmt_perma_link(const blog_t * conf, struct nentry *e, char * fmt)
{
	char k[FMT_TAIA_HEX];
	fmt[0] = 0;

	fmt_key_plain(e, k);

	strcat(fmt, PROTO_HTTP);
	strcat(fmt, conf->host);
	strcat(fmt,"?ts=");
	strcat(fmt, k);
}

/* HTML */
static void print_key_html(const blog_t * conf, struct nentry *e)
{
	char buf[FMT_TAIA_HEX];
	fmt_time_hex(buf, &e->k);
	reduce_ts(buf);

	html_div_open("class", "actions");

	html_span_open("class", "k");
	html_link2("?ts=", buf, "link");
	html_span_end();

#ifdef ADMIN_MODE_PASS
	if (conf->auth) {
		html_span_open("class", "k");
		html_link2("?del=", buf, "delete");
		html_span_end();

		html_span_open("class", "k");
		html_link2("?mod=", buf, "modify");
		html_span_end();
	}
#endif

	html_div_end();
}

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
void print_tiny_html_editor()
{
	sprintm("<script type=\"text/javascript\">"
		"new TINY.editor.edit('editor',{"
		"	id:'input',"
		"	width:700,"
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
		"	resize:{cssclass:'resize'}" "});" "</script>");
}
#else
void print_tiny_html_editor()
{
}
#endif

static void print_date_html(struct day_entry *e)
{
	char dfmt[CALDATE_FMTN];
	size_t dlen;
	dlen = caldate_fmtn(dfmt, &e->time.date);
	dfmt[dlen] = '\0';
	html_tag_open_close("h3", html_content, dfmt);
}

#ifdef WANT_ERROR_PRINT
static void print_notice_html(const blog_t * conf)
{
	switch (gerr.type) {
	case N_ERROR:
		html_div_open("id", "error");
		html_content_m("Error: ", gerr.note, " ");
//              "<div id=\"body\">", strerror(gerr.error), "</div>",
		html_content("Sys error");
		html_div_end();
		break;
	case N_NOTE:
		html_div_open("id", "note");
		html_content_m("Note: ", gerr.note);
		html_div_end();
		break;
	case N_ACTION:
		html_div_open("id", "note");
		html_content(gerr.note);
		html_div_end();
		break;
	default:
		return;
	}
}
#else
static void print_notice_html(const blog_t * conf)
{
}
#endif



static void print_header_html(const blog_t * conf)
{
	/* set a cookie */
	if(conf->csstype == CSS_SELECT)
		http_set_cookie("css", conf->css, "");
	if(conf->csstype == CSS_RESET)
		http_set_cookie("css", "", "");

#ifdef ADMIN_MODE_PASS
	if (conf->qry.action == QA_LOGOUT
				||((conf->authtype == AUTH_SID) && !conf->auth)) {
		http_set_cookie("sid", "", "");
	}

	if (conf->authtype == AUTH_POST) {
		http_set_cookie_ssl_age("sid", conf->sessionid, SESSION_STR_VTIME);
	}
#endif

	html_http_header();
	html_print_preface();

	/* stylesheet */
	if (conf->csstype == CSS_SELECT || conf->csstype == CSS_COOKIE)
		html_link_css(conf->css);
	else
		html_link_css(DEFAULT_STYLESHEET);

	/* rss */
	html_link_rss(conf->title, "?fmt=rss");

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		html_link_css(TINY_HTML_PATH"style.css");
		html_java_script(TINY_HTML_PATH "packed.js");
	}
#endif
#if defined(ADMIN_MODE) && defined(WANT_UPLOAD)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		html_java_script(UPLOAD_JS);
	}
#endif
#ifdef ADMIN_MODE_PASS
	if (!conf->ssl && (conf->auth || conf->qry.action > 0)) {
		html_meta_refresh(PROTO_HTTPS, conf->host, "3");
	}
#endif
	html_title(conf->title);
	html_close_head_body("class=\"home blog\">");

	html_div_open("id", "body");
	html_div_open("id", "wrapper");
	html_div_open("id", "wrapper2");

	html_tag_open_close2("h1", html_link, "/", conf->title);

#ifdef ADMIN_MODE_PASS
	if(conf->auth){

		html_div_open("class", "mactions");
		html_content("(");
		if (conf->qry.action != QA_ADD ) {
			html_link2(conf->script, "?add", "Add entry");
		}

#ifdef ADMIN_MODE_PASS
		html_link2(conf->script, "?logout", "Logout");
		html_content(")");
		html_div_end();
	}
#endif
#endif

	if(strcmp(conf->tagline, "")){
		html_div_open("id", "tagline");
		html_content(conf->tagline);
		html_div_end();
	}
	html_div_open2("id", "content", "class", "box shadow opacity");
	html_div_inc();

#ifdef ADMIN_MODE_PASS
	if (!conf->ssl && (conf->auth || conf->qry.action > 0)) {
		html_content("Need ssl connetion");
		html_close_body();
		exit(1);
	}
#endif

	print_notice_html(conf);
}

static void day_entries_html(const blog_t * conf, struct day_entry *de,
	size_t elen)
{
	int i;
	struct nentry *e;

	html_div_open("class", "day");
	print_date_html(de);
	html_tag_open("ul");
	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
		html_tag_open("li");

		array_cat0(&e->e);
		html_span_open("class", "c");
		html_content(e->e.p);
		html_span_end();

		print_key_html(conf, e);
		html_tag_close("li");
	}
	html_tag_close("ul");
	html_div_end();
}

static void print_footer_html(const blog_t * conf)
{
#ifdef WANT_MONTH_BROWSING
	/* TODO optimize */
	/* print month selection */
	char older[FMT_CALDATE];
	char nower[FMT_CALDATE];
	char newer[FMT_CALDATE];
	struct caltime ct;
	memcpy(&ct, &conf->qry.mon, sizeof(struct caltime));

	nower[caldate_fmt(nower, &ct.date)] = 0;
	nower[str_rchr(nower, '-')] = 0;

	ct.date.month--;
	caldate_normalize(&ct.date);
	older[caldate_fmt(older, &ct.date)] = 0;
	older[str_rchr(older, '-')] = 0;

	ct.date.month += 2;
	caldate_normalize(&ct.date);
	newer[caldate_fmt(newer, &ct.date)] = 0;
	newer[str_rchr(newer, '-')] = 0;

	html_div_open("id", "nav");
	html_link2("?mn=", older, "older");
	html_content(" - ");
	html_link2("?mn=", nower, "whole month");
	html_content(" - ");
	html_link2("?mn=", newer, "later");
	html_div_end();

	html_div_close_all();
	html_close_body();
#endif
}

#ifdef ADMIN_MODE
/*  HTML MODE ONLY */

int print_config(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Configuration", 0, N_ACTION);
	print_notice_html(conf);

	html_div_open("id", "conf");
	html_form_open("post", conf->script, 0 , 0);
	html_input("hidden", "action", "config");

	html_tag_open("p");
	html_content("Title: ");
	html_input("text", "title", conf->title);
	html_tag_close("p");

	html_tag_open("p");
	html_content("Tagline: ");
	html_input("text", "tagline", conf->tagline);
	html_tag_close("p");

	html_tag_open("p");
	html_content("Password: ");
	html_input("password", "input", "");
	html_tag_close("p");

	html_div_open("class", "abutton");
	html_input("submit", NULL, "Save");
	html_div_end();

	html_div_end();
	html_form_close();

	print_footer_html(conf);
	return 0;
}

void print_upload(const blog_t * conf)
{
#if defined(ADMIN_MODE) && defined(WANT_UPLOAD)
	if (
#ifdef ADMIN_MODE_PASS
			conf->auth &&
#endif
			(conf->qry.action == QA_MODIFY || conf->qry.action ==
			QA_ADD)){

	html_div_open("id", "upload");

	html_form_open("post", UPLOAD_CGI,
			"multipart/form-data",
			"return AIM.submit(this, {'onStart' : startCallback, 'onComplete' : completeCallback})");

	html_div_open(0, 0);
	html_tag_open("label");
	html_content("File:");
	html_input("file", "file", 0);
	html_div_end();

	html_div_open(0, 0);
	html_input("submit", 0, "Upload");
	html_div_end();

	html_form_close();

	html_div_open("id", "upload_out");
	html_span_open("id", "nr");
	html_content("0");
	html_span_end();
	html_content("Files uploaded");
	html_div_end();

	html_div_open(0, 0);
	html_content("Filename: ");
	html_bulk("<pre id=\"r\">");
	html_div_end();
	}
#endif
}

int print_add_entry(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Add a new entry", 0, N_ACTION);
	print_notice_html(conf);

	html_div_open("id", "mod");
	html_form_open("post", conf->script, 0, "editor.post();");
	html_input("hidden", "action", "add");

	html_textarea_open("input", "input");
	html_textarea_close();

	html_div_open("class", "abutton");
	html_input("submit", NULL, "Add");
	html_div_end();

	html_form_close();

	print_tiny_html_editor();
	print_upload(conf);
	html_div_end();

	print_footer_html(conf);
	return 0;
}
int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	char key[FMT_TAIA_HEX];

	fmt_key_plain(n, key);

	print_header_html(conf);

	/* set appropriate notice */
	set_err("Modify an entry", 0, N_ACTION);
	print_notice_html(conf);

	html_div_open("id", "mod");
	html_form_open("post", conf->script, 0, "editor.post();");

	html_input("hidden", "action", "mod");
	html_input("hidden", "key", key);

	html_textarea_open("input", "input");
	if (n->e.p)
		html_content(n->e.p);
	else
		html_content("Entry not found");
	html_textarea_close();

	print_tiny_html_editor();

	html_div_open("class", "abutton");
	html_input("submit", 0, "Modify");
	html_div_end();

	html_form_close();

	print_upload(conf);
	html_div_end();

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

	html_div_open("id", "login");

	html_form_open("post", conf->script, 0 ,0 );
	html_content("Enter your password: ");
	html_bulk("<input name=\"login\" type=\"password\" size=\"12\" maxlength=\"12\">\n");

	html_input("submit", 0, "Login");
	html_form_close();
	html_div_end();

	print_footer_html(conf);
}

#endif
/* RSS */

/*  static void print_date_rss(struct day_entry *de)
{
	//TODO
}*/
static inline void print_header_rss(const blog_t * conf)
{
	rss_http_header();
	rss_print_preface(conf->title, PROTO_HTTP, conf->host, conf->title, PROGRAM_NAME);
}


/* needs at most strlen(s) + 1 */


static void day_entries_rss(const blog_t * conf, struct day_entry *de,
	size_t elen)
{
	int i;
	struct nentry *e;

	char lnk[256] = "";
	char * cnt;

	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
		cnt = alloca(array_bytes(&e->e));

		fmt_perma_link(conf, e, lnk);
		http_remove_tags(e->e.p, -1, cnt);

		rss_item(cnt, lnk, e->e.p);
	}
}

static void print_footer_rss(const blog_t * conf)
{
	rss_close_rss();
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
		http_not_found();
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
	/* todo make generic */
	if (i == 0) {
		html_div_open("class", "day");
		html_tag_open_close("h3", html_content, "No entries found");
		html_div_end();
	}

	if (conf->fmt->footer)
		conf->fmt->footer(conf);
}
