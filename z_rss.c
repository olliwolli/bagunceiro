#include <string.h>

#include "z_features.h"
#include "z_http.h"

void rss_http_header()
{
	http_content_type("application/rss+xml");
}

void rss_print_preface(const char *t, const char *p, const char *l,
	const char *d, const char *g)
{
	sprintm("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
		"<rss version=\"2.0\">\n"
		"<channel>\n",
		"<title>",
		t,
		"</title>\n"
		"<link>", p,
		l,
		"</link>\n"
		"<description>",
		t,
		"</description>\n"
		"<docs>http://blogs.law.harvard.edu/tech/rss</docs>\n"
		"<generator>", g, "</generator>\n\n");
}

void rss_close_rss()
{
	sprint("\n</channel></rss>");
}

void rss_item(const char *c, const char *l, const char *d, const char *w)
{
	sprintm("<item>\n" "<title>", c,
		"</title>\n" "<link>", l,
		"</link>\n" "<description><![CDATA[", c, "]]></description>\n");
	if (strlen(w))
		sprintm("<pubDate>", w, "</pubDate>\n");

	sprintm("<guid>", l, "</guid>\n" "</item>\n\n");
}
