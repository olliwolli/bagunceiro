#define PROTO_HTTP "http://"
#define PROTO_HTTPS "https://"

void http_headers(const char * t);
void http_set_cookie(const char * k, const char * v, const char * o);
void http_set_cookie_ssl_age(const char * k, const char * v, const char *a);
void http_not_found();
