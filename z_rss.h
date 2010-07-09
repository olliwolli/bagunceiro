#ifndef _Z_RSS_H
#define _Z_RSS_H

void rss_http_header();
void rss_print_preface(const char * t, const char *p, const char *l, const char * d, const char *g);
void rss_close_rss();
/* c=title, l=link, d=description, w=pubDate */
void rss_item(const char * c, const char * l, const char * d, const char *w);

#endif
