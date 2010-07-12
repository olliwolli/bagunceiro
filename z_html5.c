#include "z_features.h"
#include "z_http.h"

/* helper */
static int divstack;
static int indent;

/* not perfect, but whatever */

#define conprint(str) do{\
						indent++;\
						indent_space(indent);\
						sprint(str);\
						indent--;\
						sprintn("\n", 1);\
					}while(0)

#define oprint(str) do{\
						indent++;\
						indent_space(indent);\
						sprint(str);\
						sprintn("\n", 1);\
					}while(0)

#define oprintm(...) do{\
						indent++;\
						indent_space(indent);\
						sprintm( __VA_ARGS__ );\
						sprintn("\n", 1);\
					}while(0)

#define cprint(str) do{\
						indent_space(indent);\
						sprint(str);\
						indent--;\
						sprintn("\n", 1);\
					}while(0)

#define cprintm(...) do{\
						indent_space(indent);\
						sprintm( __VA_ARGS__ );\
						indent--;\
						sprintn("\n", 1);\
					}while(0)

void indent_space(int i)
{
	if(i<=0)
		return;
	while(i--)
		sprintn("  ", 2);
}

void html_bulk(const char *s)
{
	sprint(s);
}

void html_content(const char *c)
{
	conprint(c);
}

void html_div_open(char *type, char *name)
{
	if(!type || ! name)
		oprint("<div>");
	else
		oprintm("<div ", type, "=\"", name, "\">");
	divstack++;
}

void html_div_open2(const char *type, const char *name, const char * type2, const char *name2)
{
	if(!type || ! name)
		oprint("<div>");
	else
		oprintm("<div ", type, "=\"", name, "\" ", type2, "=\"", name2, "\">");
	divstack++;
}

void html_div_inc()
{
	divstack++;
}

void html_div_end()
{
	cprint("</div>");
	divstack--;
}

void html_span_open(const char * type, const char * name)
{
	oprintm("<span ", type, "=\"", name, "\">");
}

void html_span_end()
{
	cprint("</span>");
}

void html_div_close_all()
{
	while (divstack-- > 0)
		cprint("</div>");
}

void html_http_header()
{
	divstack = indent = 0;
	http_content_type("text/html");
}

void html_print_preface()
{
	sprintm("<!doctype html>\n"
			"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\" />\n"
			"<html>\n<head>\n");
}

void html_link_rss(const char * t, const char * h)
{
	sprintm("<link rel=\"alternate\" type=\"application/rss+xml\" title=\"",
			t, " in rss\" href=\"", h ,"\">\n");
}

void html_link_css(const char * s)
{
	sprintm("<link rel=stylesheet type=\"text/css\" href=\"");
	if(s[0] != '/')
		sprint("/");
	sprintm(s, "\" >\n");
}

void html_java_script(const char *s)
{
	sprintm("<script type=\"text/javascript\" src=\"", s ,"\"></script>\n\n");
}

void html_meta_refresh(const char * p, const char *url , const char *t)
{
	sprintm("<META HTTP-EQUIV=\"Refresh\"",
      "CONTENT=\"", t ,"; URL=", p, url ,"\">");
}

void html_title(const char *t)
{
	oprint("<title>");
	html_content(t);
	cprint("</title>");
}

void html_close_head_body(const char * b)
{
	sprintm("</head>\n\n" "<body ", b,"\n");
}

void html_close_body()
{
	sprint("</body>\n" "</html>\n");
}

void html_tag_open(const char *t)
{
	if(t)
		oprintm("<", t, ">");
	sprint("");
}

void html_tag_close(const char *t)
{
	if(t)
		cprintm("</", t, ">");
}

void html_tag_open_close(const char *t, void(*f)(const char * v), const char *v)
{
	html_tag_open(t);
	f(v);
	html_tag_close(t);
}


void html_tag_open_close2(const char *t, void(*f)(const char * v1v, const char *v2v), const char *v1, const char *v2)
{
	html_tag_open(t);
	f(v1, v2);
	html_tag_close(t);
}

void html_link(const char * l, const char * t)
{
	oprintm("<a href=\"", l, "\">");
	html_content(t);
	cprintm("</a>");
}

void html_link2(const char * l1, const char * l2, const char * t)
{
	oprintm("<a href=\"", l1, l2, "\">");
	html_content(t);
	cprint("</a>");
}

void html_input(const char * type, const char* name, const char* value)
{
	indent++;
	indent_space(indent);
	sprintm("<input type=\"", type ,"\" " );
	if(name)
		sprintm("name=\"", name, "\" ");
	if(value)
		sprintm("value=\"",value,"\" ");
	sprint(">\n");
	indent--;
}

void html_textarea_open(const char *name, const char * id)
{
	indent++;
	indent_space(indent);
	sprintm("<textarea name=\"",name,"\" id=\"",id, "\" >");
}

void html_textarea_close()
{
	sprint("</textarea>\n");
	indent--;
}

void html_checkbox(const char *name, const char *v, int checked)
{
	sprintm("<input type=\"checkbox\" name=\"",name,"\" value=\"",v, "\"");
	if(checked)
		sprint(" checked");
	sprint(">");
}

void html_form_close()
{
	cprint("</form>");
}

void html_form_open(const char * m, const char * a, const char *enc, const char *os)
{
	indent++;
	indent_space(indent);

	sprintm("<form method=\"", m, "\" action=\"", a, "\"");

	if(enc)
		sprintm(" enctype=\"", enc, "\"");

	if(os)
		sprintm(" onsubmit=\"", os, "\"");

	sprint(" >\n");
	indent--;
}

void http_remove_tags(char *s, size_t n, char *fmt)
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
						fmt[i] = ' ';
						sprintn(" ", 1);
						ws = 1;
						i++;
					}
				} else {
					fmt[i] = s[l];
					ws = 0;
					i++;
				}
			}
		}
		fmt[i] = '\0';
}
