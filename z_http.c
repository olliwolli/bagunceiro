#include "z_features.h"
#include "z_http.h"

void http_content_type(const char * t)
{
	sprintm("Content-type: ", t ,"; charset=UTF-8\r\n\r\n");
}

void http_set_cookie(const char * k, const char * v, const char * o)
{
	sprintm("Set-Cookie: ",k ,"=");
	sprint(v);
	sprintm(";", o, "\r\n");
}

void http_set_cookie_ssl_age(const char * k, const char * v, const char * a)
{
	sprintm("Set-Cookie: ",k ,"=");
	sprint(v);
	sprintm(";", "Secure; HttpOnly; Discard; Max-Age=", a, "\";\r\n");
}

#ifdef WANT_HTTP_304
void http_last_modified(time_t t)
{
	char fmt[FMT_HTTPDATE];
	fmt[fmt_httpdate(fmt, t)] = 0;
	sprintm("Last-Modified: ", fmt,"\r\n");
}

void http_not_changed_modified()
{
	sprint("Status: 304 Not Modified\r\n\r\n");
}
#endif

void http_not_found()
{
	sprint("Status: 404 Bad Request\r\n\r\n");
}
