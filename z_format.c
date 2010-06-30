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

/* helper */
static int divstack;

static inline void startdiv(char *type, char *name)
{
	sprintm("<div ", type, "=\"", name, "\">\n");
	divstack++;
}

static inline void enddiv()
{
	sprint("</div>");
	divstack--;
}

static inline void closedivs()
{
	while (divstack-- > 0)
		sprint("</div>\n");
}

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
	sprintm("http://", conf->host, "?ts=");
	print_key_plain(e);
}

/* HTML */
static void print_key_html(const blog_t * conf, struct nentry *e)
{
	char buf[FMT_TAIA_HEX];
	fmt_time_hex(buf, &e->k);
	reduce_ts(buf);

	startdiv("class", "actions");

	sprintm(" <span class=\"k\"><a href=\"" "?ts=",
		buf, "\">link</a></span> ");

#ifdef ADMIN_MODE
	if (conf->auth) {
		sprintm(" <span class=\"k\"><a href=\"" "?del=",
			buf, "\">delete</a></span> ");

		sprintm(" <span class=\"k\"><a href=\"" "?mod=",
			buf, "\">modify</a></span> ");
	}
#endif

	enddiv();
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
	sprintm("<h3>", dfmt, "</h3>\n");
}

#ifdef WANT_ERROR_PRINT
static void print_notice_html(const blog_t * conf)
{
	switch (gerr.type) {
	case N_ERROR:
		startdiv("id", "error");
		sprintm("Error: ", gerr.note, " ");
//              "<div id=\"body\">", strerror(gerr.error), "</div>",
		sprint("Sys error");
		enddiv();
		break;
	case N_NOTE:
		startdiv("id", "note");
		sprintm("Note: ", gerr.note);
		enddiv();
		break;
	case N_ACTION:
		startdiv("id", "note");
		sprint(gerr.note);
		enddiv();
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

static void set_cookie(const char * k, const char * v, const char * o)
{
	sprintm("Set-Cookie: ",k ,"=");
	sprint(v);
	sprintm(";", o, "\n");
}

static void set_stylesheet(const char * s)
{
	sprintm("<link rel=stylesheet type=\"text/css\" href=\"");
	if(s[0] != '/')
		sprint("/");
	sprintm(s, "\" >\n");
}



static void print_header_html(const blog_t * conf)
{

	/* set a cookie */
	if(conf->csstype == CSS_SELECT)
		set_cookie("css", conf->css, "");
	if(conf->csstype == CSS_RESET)
		set_cookie("css", "", "");

#ifdef ADMIN_MODE_PASS
	if (conf->qry.action == QA_LOGOUT
			//TODO uncomment!!!  ||((conf->authtype == AUTH_SID) && !conf->auth)
			) {
		set_cookie("sid", "", "");
	}

	if (conf->authtype == AUTH_POST) {
		set_cookie("sid", conf->sessionid,"Secure; HttpOnly; Discard; Max-Age="SESSION_STR_VTIME";");
	}
#endif

	sprintm(CONTENT_TYPE_HTML, DOCTYPE);
	sprint("<meta http-equiv=\"Content-type\""
			"content=\"text/html;charset=UTF-8\" />");
	sprint("<html>" "<head>");

	/* stylesheet */
	if (conf->csstype == CSS_SELECT || conf->csstype == CSS_COOKIE)
		set_stylesheet(conf->css);
	else
		set_stylesheet(DEFAULT_STYLESHEET);

	/* rss */
	sprintm("<link rel=\"alternate\" type=\"application/rss+xml\" title=\"",
		conf->title, " in rss\" href=\"?fmt=rss\">\n");

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		set_stylesheet(TINY_HTML_PATH"style.css");
		sprintm("<script type=\"text/javascript\" src=\""
			TINY_HTML_PATH "packed.js\"></script>\n\n");
	}
#endif
#if defined(ADMIN_MODE) && defined(WANT_UPLOAD)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		sprintm("<script type=\"text/javascript\" src=\"", UPLOAD_JS, "\"></script>");
	}
#endif
#ifdef ADMIN_MODE_PASS
	if (!conf->ssl && (conf->auth || conf->qry.action > 0)) {
		sprintm("<META HTTP-EQUIV=\"Refresh\""
	      "CONTENT=\"3; URL=https://",conf->host ,"\">");
	}
#endif
	sprintm("<title>", conf->title, "</title>\n");

	sprintm("</head>\n\n" "<body class=\"home blog\">\n");
	startdiv("id", "body");
	startdiv("id", "wrapper");
	startdiv("id", "wrapper2");
	sprintm("<h1><a href=\"/\">", conf->title, "</a></h1>\n\n");
#ifdef ADMIN_MODE
	sprint("(");
	startdiv("class", "mactions");
	if (conf->auth && conf->qry.action != QA_ADD ) {
		sprintm("<a href=\"", conf->script,
			"?add\">" "Add entry " "</a>\n");
	}
#ifdef ADMIN_MODE_PASS
	if (conf->auth){
		sprintm("<a href=\"", conf->script, "?");
		sprint("logout\">" "Logout");
		sprint("</a>\n");
	}
	enddiv();
	sprint(")");
#endif
#endif

	if(strcmp(conf->tagline, "")){
		sprintm("<div id=\"tagline\">",conf->tagline ,"</div>");
	}
	sprint("<div id=\"content\" class=\"box shadow opacity\">\n\n");
	divstack++;

#ifdef ADMIN_MODE_PASS
	if (!conf->ssl && (conf->auth || conf->qry.action > 0)) {
		sprintf("Need ssl connetion");
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

	startdiv("class", "day");
	print_date_html(de);
	sprint("<ul>\n");
	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
		sprint(" <li>");
		array_cat0(&e->e);

		sprintm("<span class=\"c\">", e->e.p, "</span>");

		print_key_html(conf, e);
		sprint("</li>\n");
	}
	sprint("</ul>\n");
	enddiv();
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

	startdiv("id", "nav");
	sprintm("\n <a href=\"?mn=", older, "\">older</a> -");
	sprintm("- <a href=\"?mn=", nower, "\">whole month</a>\n");
	sprintm("- <a href=\"?mn=", newer, "\">later</a>\n");
	enddiv();
//      sprintmf("<h4>Please Note: ...  </span></h4>"
	closedivs();
	sprintf("</body>\n" "</html>\n");
#endif
}

#ifdef ADMIN_MODE
/*  HTML MODE ONLY */

static void print_input(const char * type, const char* name, const char* value){
	sprintm("<input type=\"", type ,"\" " );
	if(name)
		sprintm("name=\"", name, "\" ");
	if(value)
		sprintm("value=\"",value,"\" ");
	sprint(">\n");
}

int print_config(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Configuration", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<div id=\"conf\">\n"
		"<form  method=\"post\" action=\"",conf->script,"\">\n");
	print_input("hidden", "action", "config");
	sprint("<p>Title: ");
	print_input("text", "title", conf->title);
	sprint("</p>");
	sprint("<p>Tagline: ");
	print_input("text", "tagline", conf->tagline);
	sprint("</p>");
	sprint("<p>Password: ");
	print_input("password", "input", "");
	sprint("</p>");
	sprint("<div class=\"abutton\">\n");
	print_input("submit", NULL, "Save");
	sprint("</div>");
	sprint("</form>\n");

	print_footer_html(conf);
	return 0;
}

void print_upload(const blog_t * conf)
{
#if defined(ADMIN_MODE) && defined(WANT_UPLOAD)
	if (conf->auth && (conf->qry.action == QA_MODIFY || conf->qry.action ==
			QA_ADD)){
	sprintmf("<div id=\"upload\"><form enctype=\"multipart/form-data\" action=\"",UPLOAD_CGI,"\"\n"
		"method=\"post\"\n"
		"onsubmit=\"return AIM.submit(this, {'onStart' : startCallback, 'onComplete' : completeCallback})\">\n"
		"<div><label>File:</label> <input type=\"file\" name=\"file\" /></div>\n"
		"<div><input type=\"submit\" value=\"Upload\" /></div>\n"
	"</form><div id=\"upload_out\"><span id=\"nr\">0</span> Files uploaded</div>\n"
	"<div>Filename: <pre id=\"r\"></div>"
	);
	}
#endif

}

int print_add_entry(const blog_t * conf)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Add a new entry", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<div id=\"mod\">\n"
		"<form  onsubmit='editor.post();' method=\"post\" action=\"",
		conf->script,
		"\">\n");
	print_input("hidden", "action", "add");
	sprint("<textarea name=\"input\" id=\"input\" style=\"width:400px; height:200px\"></textarea>\n");
	sprint("");
	sprint("<div class=\"abutton\">\n");
	print_input("submit", NULL, "Add");
	sprint("</div>");
	sprint("</form>\n");

	print_tiny_html_editor();
	print_upload(conf);
	sprint("</div>");
	print_footer_html(conf);
	return 0;
}
int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	print_header_html(conf);

	/* set appropriate notice */
	set_err("Modify an entry", 0, N_ACTION);
	print_notice_html(conf);

	sprintm("<div id=\"mod\">\n"
		"<form  onsubmit='editor.post();' method=\"post\" action=\"",
		conf->script, "\">\n",
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

	sprint("<div class=\"abutton\">");
	sprintm("<input type=\"submit\" value=\"Modify\">");
	sprint("</div>");
	sprint(	"</form>\n");

	print_upload(conf);
	sprint("</div>");
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

	sprintm("<div id=\"login\">"
		"<form  method=\"post\" action=\"", conf->script, "\">\n",
		"Enter your password: <input name=\"login\" type=\"password\" size=\"12\" maxlength=\"12\">\n");

	sprintm("<input type=\"submit\" value=\"Login\">" "</form></div>");

	print_footer_html(conf);
}

#endif
/* RSS */
/*  static void print_date_rss(struct day_entry *de)
{
	//TODO
}*/

#define CONTENT_TYPE_RSS "Content-type: application/rss+xml; charset=UTF-8\r\n" "\r\n"
static void print_header_rss(const blog_t * conf)
{
	sprintm(CONTENT_TYPE_RSS
		"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<rss version=\"2.0\">\n"
		"<channel>\n",
		"<title>",
		conf->title,
		"</title>\n"
		"<link>http://",
		conf->host,
		"</link>\n"
		"<description>",
		conf->title,
		"</description>\n"
		"<docs>http://blogs.law.harvard.edu/tech/rss</docs>\n"
		"<generator>", PROGRAM_NAME, "</generator>\n\n");

}

static inline void print_wo_tags(char *s, size_t n)
{
	int tagopen, andopen;
	int i, l, ws;

	tagopen = 0;
	andopen = 0;
	ws = 0;

	/* remove html artifacts */
	/* FIXME rss also does not like spaces at the end of an entry */
	for (l = i = 0; i < n && s[l] != '\0'; l++) {
		if (s[l] == '<')
			tagopen++;
		else if (s[l] == '>' && tagopen)
			tagopen = 0;
		else if (s[l] == '&')
			andopen = 1;
		else if (s[l] == ';' && andopen)
			andopen = 0;
		else if (tagopen == 0 && andopen == 0) {
			if ((s[l] == '\n' || s[l] == '\r' || s[l] == ' ')) {
				if (!ws) {
					sprintn(" ", 1);
					ws = 1;
				}
			} else {
				sprintn(&s[l], 1);
				ws = 0;
			}
			i++;
		}
	}

}

static void day_entries_rss(const blog_t * conf, struct day_entry *de,
	size_t elen)
{
	int i;
	struct nentry *e;

	for (i = 0; i < elen; i++) {
		e = array_get(&de->es, sizeof(struct nentry), i);
		sprintm("<item>\n" "<title>");
		print_wo_tags(e->e.p, -1);
		sprintm("</title>\n" "<link>");
		print_perma_link(conf, e);

		sprintm("</link>\n" "<description><![CDATA[",
			e->e.p, "]]></description>\n");
		//      sprint("<pubDate>");
		//      print_date_rss(de);
		//      sprintm("</pubDate>\n"
		sprint("<guid>");
		print_perma_link(conf, e);
		sprintm("</guid>\n" "</item>\n\n");
	}
}

static void print_footer_rss(const blog_t * conf)
{
//      sprintm("<atom:link href=\"http://",conf->host,"?fmt=rss\" rel=\"self\" type=\"application/rss+xml\" />");
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
	if (i == 0) {
		sprint("<div class=\"day\"><h3>No entries found</h3></div>");
	}

	if (conf->fmt->footer)
		conf->fmt->footer(conf);
}
