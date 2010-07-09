#ifndef _Z_HTML5_H
#define _Z_HTML5_H

#define html_content_m(...) buffer_putm(buffer_1, ##__VA_ARGS__)

void html_bulk(const char *s);
void html_content(const char *c);

void html_div_open(char *type, char *name);
void html_div_open2(const char *type, const char *name, const char * type2, const char *name2);
void html_div_end();
void html_div_close_all();
void html_div_inc();

void html_span_open(const char * type, const char * name);
void html_span_end();

void html_http_header();
void html_print_preface();

void html_link_rss(const char * t, const char * h);
void html_link_css(const char * s);
void html_java_script(const char *s);
void html_meta_refresh(const char *url, const char * p, const char *t);

void html_title(const char *t);
void html_close_head_body(const char * b);
void html_close_body();

void html_link(const char * l, const char * t);
void html_link2(const char * l1, const char * l2, const char * t);

void html_tag_open(const char *t);
void html_tag_close(const char *t);

void html_tag_open_close(const char *t, void(*f)(const char * v), const char *v);
void html_tag_open_close2(const char *t, void(*f)(const char * v1v, const char *v2v), const char *v1, const char *v2);

void html_form_open(const char * m, const char * a, const char *enc, const char *os);
void html_checkbox(const char *name, const char *v, int checked);
void html_form_close();
void html_input(const char * type, const char* name, const char* value);
void html_textarea_open(const char *name, const char * id);
void html_textarea_close();

void http_remove_tags(char *s, size_t n, char *fmt);

#endif
