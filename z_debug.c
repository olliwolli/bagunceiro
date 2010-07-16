#include <array.h>

#include "z_debug.h"

/* DEBUGGING */
#ifdef DEBUG

char debug_query[] = "";
char debug_post[] = "";
char debug_cookie[] = "";

array debugmsg;
char daction[4][10] = { "Show", "Delete", "Add", "Modify" };

void __d(const char *desc, const char *value)
{
	array_cats(&debugmsg, "<tr>");
	if (value) {
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, "</th>");
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, value);
		array_cats(&debugmsg, "</th>");
	} else {
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, desc);
		array_cats(&debugmsg, "</th>");
		array_cats(&debugmsg, "<th align=\"left\">");
		array_cats(&debugmsg, "[Empty]<br>\n");
		array_cats(&debugmsg, "</th>");
	}
	array_cats(&debugmsg, "</tr>");
}

void debug_print(const blog_t * conf, array * ps, array * qs, array * co)
{
	array_cats(&debugmsg, "<table>");
	__d("Poststring is: ", ps->p);
	__d("Querystring is: ", qs->p);
	__d("Cookie is: ", co->p);
	__d("URL is: ", conf->script);
	__d("Host is: ", conf->host);
	__d("CSS arg is: ", conf->qry.css);

	if (array_bytes(&conf->qry.input))
		__d("Input is: ", conf->qry.input.p);

	__d("Key is: ", conf->qry.ts);

	__d("Action is: ", daction[conf->qry.action]);
	array_cats(&debugmsg, "</table>");
}
#endif
