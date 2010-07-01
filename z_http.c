#include "z_features.h"

void http_headers(const char * t)
{
	sprintm("Content-type: ", t ,"; charset=UTF-8\r\n\r\n");
}

void http_set_cookie(const char * k, const char * v, const char * o)
{
	sprintm("Set-Cookie: ",k ,"=");
	sprint(v);
	sprintm(";", o, "\n");
}

void http_set_cookie_ssl_age(const char * k, const char * v, const char * a)
{
	sprintm("Set-Cookie: ",k ,"=");
	sprint(v);
	sprintm(";", "Secure; HttpOnly; Discard; Max-Age=", a, "\";\n");
}

void http_not_found()
{
	sprint("Status: 404 Bad Request\r\n\r\n");
}
