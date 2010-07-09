#ifdef WANT_FAST_CGI
	#include "fcgi_config.h"
	#include "fcgiapp.h"

	FCGX_Stream *fcgi_in, *fcgi_out, *fcgi_err;
	FCGX_ParamArray envp;

	#define getenv(str) FCGX_GetParam((str), envp)
	#define FCGX_PutSm(b,...) FCGX_PutSm_internal(b,__VA_ARGS__,(char*)0)

	#define sprint(str) FCGX_PutS(str, fcgi_out)
	#define sprintm(...) FCGX_PutSm(fcgi_out, ##__VA_ARGS__)
	#define sprintf(str) FCGX_PutS(str, fcgi_out)
	#define sprintmf(...) FCGX_PutSm(fcgi_out, ##__VA_ARGS__)
	#define sprintn(str, len) FCGX_PutStr(str, len, fcgi_out)

	#ifdef WANT_ERROR_PRINT
	#define eprint(str) FCGX_PutS(str, fcgi_err)
	#define eprintm(...) FCGX_PutSm(fcgi_err, ##__VA_ARGS__)
	#define eprintf(str) do {FCGX_PutS(str, fcgi_err);} while(0)
	#define eprintmf(...) do{FCGX_PutSm(fcgi_err);} while(0)
	#define eprintn(str, len) FCGX_PutStr(str, len, fcgi_err)
	#endif
#endif
