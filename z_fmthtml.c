#include <string.h>

#include <textcode.h>
#include <taia.h>

#include "z_cdbb.h"
#include "z_day.h"
#include "z_blog.h"
#include "z_html5.h"
#include "z_http.h"
#include "z_fmthtml.h"
#include "z_format.h"

/* HTML */
static void print_key_html(const blog_t * conf, struct nentry *e)
{
	char buf[FMT_TAIA_HEX];

	buf[fmt_hexdump(buf, e->k.p, TAIA_PACK)] = 0;
	reduce_ts(buf);

	html_div_open("class", "actions");

	html_span_open("class", "k");
	html_qry_param_link2(QUERY_TS, buf, "link");
	html_span_end();

#ifdef ADMIN_MODE_PASS
	if (conf->auth) {
		html_span_open("class", "k");
		html_qry_param_link2(QUERY_DEL, buf, "delete");
		html_span_end();

		html_span_open("class", "k");
		html_qry_param_link2(QUERY_MOD, buf, "modify");
		html_span_end();
	}
#endif

	html_div_end();
}

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
static void print_tiny_html_editor()
{
	html_java_script("/tinymce/tiny_mce.js");
	sprintm("<script type=\"text/javascript\">");
	sprintm(
		"tinyMCE.init({"
		"mode : \"textareas\","
		"theme : \"advanced\","
		"theme_advanced_buttons1 : \"bold,italic,underline,strikethrough,|,justifyleft,justifycenter,justifyright,justifyfull,|,styleselect,formatselect,fontselect,fontsizeselect|,forecolor,backcolor\","
		"theme_advanced_buttons2 : \"cut,copy,paste,pastetext,|,bullist,numlist,|,outdent,indent,blockquote,|,undo,redo,|,link,unlink,anchor,image,cleanup,code,removeformat,visualaid,|,sub,sup,|,charmap\","
		"theme_advanced_buttons3 : \"\","
		"theme_advanced_toolbar_location : \"top\","
		"theme_advanced_toolbar_align : \"left\","
		"theme_advanced_statusbar_location : \"bottom\","
		"theme_advanced_resizing : \"true\","
		"forced_root_block : false,"
		"force_br_newlines : true,"
		"force_p_newlines : false,"
		"plugins : \"inlinepopups,paste\","
		"paste_auto_cleanup_on_paste : true,"
		"paste_remove_styles: true,"
		"paste_strip_class_attributes: true,"
		"paste_retain_style_properties: \"\","
		"paste_remove_spans: true,"
		"paste_remove_styles_if_webkit: true,"
		"dialog_type : \"modal\""
	"});"
	"</script>"
	);
}
#endif

static void print_date_html(struct day *e)
{
	char dfmt[FMT_CALDATE_NAV];
	size_t dlen;

	dlen = fmt_caldate_nav(dfmt, &e->time.date);
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

static int print_header_html(const blog_t * conf)
{
	/* set a cookie */
	if(conf->qry.csstype == CSS_SELECT)
		http_set_cookie(COOKIE_CSS, conf->qry.css, "");
	if(conf->qry.csstype == CSS_RESET)
		http_set_cookie(COOKIE_CSS, "", "");

#ifdef ADMIN_MODE_PASS
	if (conf->qry.action == QA_LOGOUT
				||((conf->authtype == AUTH_SID) && !conf->auth)) {
		http_set_cookie(COOKIE_SID, "", "");
	}

	if (conf->authtype == AUTH_POST) {
		http_set_cookie_ssl_age(COOKIE_SID, conf->sid, SESSION_STR_VTIME);
	}
#endif

	html_http_header();
	html_print_preface();

	/* stylesheet */
	if (conf->qry.csstype == CSS_SELECT || conf->qry.csstype == CSS_COOKIE)
		html_link_css(conf->qry.css);
	else
		html_link_css(DEFAULT_STYLESHEET);

	/* rss */
	html_link_rss(conf->title, "?"QUERY_FMT"=rss");

#if defined(ADMIN_MODE) && defined(WANT_TINY_HTML_EDITOR)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		print_tiny_html_editor();
	}
#endif
#if defined(ADMIN_MODE) && defined(WANT_UPLOAD)
	if (conf->qry.action == QA_ADD || conf->qry.action == QA_MODIFY) {
		html_java_script(UPLOAD_JS);
	}
#endif
#ifdef ADMIN_MODE_PASS
	if (conf->ssl == NULL && (conf->auth || conf->qry.action > 0)) {
		html_meta_refresh(PROTO_HTTPS, conf->host, "3");
	}
#endif

	html_title(conf->title);
	html_close_head_body("class=\"home blog\">");

	html_div_open("id", "body");
	html_div_open("id", "wrapper");
	html_div_open("id", "header");
	html_tag_open_close2("h1", html_link, conf->path, conf->title);

	#ifdef ADMIN_MODE_PASS
		if(conf->ssl != NULL){
			html_div_open("class", "mactions");
			html_content("(");
			if(conf->auth){
				if (conf->qry.action != QA_ADD ) {
					html_abs_qry_link2(conf->script, QUERY_ADD, "Add entry");
				}
				html_abs_qry_link2(conf->script, QUERY_CONFIG, "Config");

	#ifdef ADMIN_MODE_PASS
				html_abs_qry_link2(conf->script, QUERY_LOGOUT, "Logout");
			}else
				html_abs_qry_link2(conf->script, QUERY_LOGIN, "Login");

			html_content(")");
			html_div_end();
		}
	#endif
	#endif

	html_div_open("id", "hwidgets");
	if(strcmp(conf->tagline, "")){
		html_div_open("id", "tagline");
		html_content(conf->tagline);
		html_div_end();
	}

#ifdef WANT_SEARCHING
	if(conf->sbox == 'y' && conf->path){
		html_div_open("id", "sbox");
		html_form_open("get", conf->path, NULL, NULL);
		html_input("text", QUERY_QRY, NULL);
		html_input("submit", NULL, "Search");
		html_form_close();
		html_div_end();
	}
#endif
	html_div_end();
	html_div_end();
	html_div_open("id", "wrapper2");
	html_div_open2("id", "content", "class", "box shadow opacity");
	html_div_inc();

#ifdef ADMIN_MODE_PASS
	if (conf->ssl == NULL && (conf->auth || conf->qry.action > 0)) {
		html_content("Need ssl connetion");
		html_close_body();
		return 1;
	}
#endif

	print_notice_html(conf);
	return 0;
}

static void day_entries_html(const blog_t * conf, struct day *de,
	size_t elen)
{
	int i;
	struct nentry *e;

	html_div_open("class", "day");
	print_date_html(de);
	html_tag_open("ul");

	for (i = 0; i < elen; i++) {
		e = day_get_nentry(de, i);
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
	/* print month selection, past, now, future */
	char p[FMT_CALDATE]= "";
	char n[FMT_CALDATE]= "";
	char f[FMT_CALDATE]= "";
	struct caltime ct;

	memcpy(&ct, &conf->qry.mon, sizeof(struct caltime));

	if(caldate_fmt(0, &ct.date) < FMT_CALDATE){
		n[caldate_fmt(n, &ct.date)] = 0;
		n[str_rchr(n, '-')] = 0;
	}

	ct.date.month--;
	caldate_normalize(&ct.date);

	if(caldate_fmt(0, &ct.date) < FMT_CALDATE){
		p[caldate_fmt(p, &ct.date)] = 0;
		p[str_rchr(p, '-')] = 0;
	}

	ct.date.month += 2;
	caldate_normalize(&ct.date);

	if(caldate_fmt(0, &ct.date) < FMT_CALDATE){
		f[caldate_fmt(f, &ct.date)] = 0;
		f[str_rchr(f, '-')] = 0;
	}

	html_div_open("id", "nav");
	html_qry_param_link2(QUERY_MONTH, p, "older");
	html_content(" - ");
	html_qry_param_link2(QUERY_MONTH, n, "whole month");
	html_content(" - ");
	html_qry_param_link2(QUERY_MONTH, f, "later");
	html_div_end();

	html_div_close_all();
	html_close_body();
#endif
}

static void print_noentries_html(const blog_t * conf)
{
	html_div_open("class", "day");
	html_tag_open_close("h3", html_content, "No entries found");
	html_div_end();
}

#ifdef ADMIN_MODE
/*  HTML MODE ONLY */

int print_config(const blog_t * conf)
{
	set_err("Configuration", 0, N_ACTION);
	print_header_html(conf);

	/* set appropriate notice */

	html_div_open("id", "conf");
	html_form_open("post", conf->script, 0 , 0);
	html_input("hidden", "action", "config");

	html_tag_open("p");
	html_content("Title: ");
	html_input("text", POST_ARG_TITLE, conf->title);
	html_tag_close("p");

	html_tag_open("p");
	html_content("Tagline: ");
	html_input("text", POST_ARG_TAGLINE, conf->tagline);
	html_tag_close("p");

	html_tag_open("p");
	html_content("Password: ");
	html_input("password", POST_ARG_PASS, "");
	html_tag_close("p");

#ifdef WANT_SEARCHING
	html_tag_open("p");
	html_content("Search box: ");
	if(conf->sbox == 'y')
		html_checkbox(POST_ARG_SEARCHBOX, "y", 1);
	else
		html_checkbox(POST_ARG_SEARCHBOX, "y", 0);
	html_tag_close("p");
#endif

	html_div_open("class", "abutton");
	html_input("submit", NULL, "Save");
	html_div_end();

	html_div_end();
	html_form_close();

	print_footer_html(conf);
	return 0;
}

static void print_upload(const blog_t * conf)
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
	html_input("file", POST_FILE_UPLOAD, 0);
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
	set_err("Add a new entry", 0, N_ACTION);
	print_header_html(conf);

	html_div_open("id", "mod");
	html_form_open("post", conf->script, 0, 0);
	html_input("hidden", "action", POST_ACTION_ADD);

	html_textarea_open(POST_ARG_INPUT, "input");
	html_textarea_close();

	html_div_open("class", "abutton");
	html_input("submit", NULL, "Add");
	html_div_end();

	html_form_close();

	print_upload(conf);
	html_div_end();

	print_footer_html(conf);
	return 0;
}

int print_mod_entry(const blog_t * conf, struct nentry *n)
{
	char key[FMT_TAIA_HEX];

	fmt_key_plain(n, key);

	set_err("Modify an entry", 0, N_ACTION);
	print_header_html(conf);

	html_div_open("id", "mod");
	html_form_open("post", conf->script, 0, 0);

	html_input("hidden", "action", POST_ACTION_MOD);
	html_input("hidden", POST_ARG_KEY, key);

	html_textarea_open(POST_ARG_INPUT, "input");
	if (n->e.p)
		html_bulk(n->e.p);
	else
		html_content("Entry not found");
	html_textarea_close();

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
	set_err("Login", 0, N_ACTION);
	if(print_header_html(conf))
		return;

	html_div_open("id", "login");

	html_form_open("post", conf->script, 0 ,0 );
	html_content("Enter your password: ");
	html_bulk("<input name=\""POST_LOGIN"\" type=\"password\" size=\"12\" maxlength=\"12\">\n");

	html_input("submit", 0, "Login");
	html_form_close();
	html_div_end();

	print_footer_html(conf);
}
#endif

struct fmting fmt__html = {
	.day_entries = day_entries_html,
	.header = print_header_html,
	.footer = print_footer_html,
	.noentries = print_noentries_html
};
