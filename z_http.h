#ifndef _Z_HTTP_H
#define _Z_HTTP_H

#define PROTO_HTTP "http://"
#define PROTO_HTTP_LEN 7
#define PROTO_HTTPS "https://"
#define PROTO_HTTPS_LEN 8

void http_content_type(const char * t);
void http_set_cookie(const char * k, const char * v, const char * o);
void http_set_cookie_ssl_age(const char * k, const char * v, const char *a);
void http_not_found();

#ifdef WANT_HTTP_304
#define FMT_HTTPDATE 30
void http_last_modified(time_t t);
void http_not_changed_modified();
#endif

#endif
