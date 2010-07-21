#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <array.h>
#include <open.h>
#include <buffer.h>
#include <str.h>
#include <textcode.h>

#include "z_features.h"

#ifdef DEBUG
#include "z_debug.h"
#endif


#ifdef WANT_FAST_CGI
#include "fcgiapp.h"
#endif

/* 300k */
#define UPLOAD_POST_LENGTH 1024 * 300
#define UPLOAD_FILENAME_LENGTH 128
#define UPLOAD_PATH "img"
#define UPLOAD_URL "img"
#define UPLOAD_PARAM "filename"

#define P_UNLIMITED -1
#define P_NO_VALUE -2
#define RET_NO_VALUE -2
#define RET_INVALID -3
#define RET_NOT_FOUND -4
#define PARAM_SUCCESS 0

static int get_post_string(array * ps)
{
	char *len;
	int plen = 0;
	memset(ps, 0, sizeof(array));

	len = getenv("CONTENT_LENGTH");
	if (len != 0) {

#ifdef WANT_FAST_CGI
		char ch[128];
		int r;
		while ((r = FCGX_GetStr(ch, 128, fcgi_in))){
			array_catb(ps, ch, r);
			if(r < 128)
				break;
		}
	return array_bytes(ps);
#else
		while (buffer_get_array(buffer_0, ps)>0) {
			plen = array_bytes(ps);
			if (plen > POSTDATA_MAX)
				break;
		}
		return plen;
#endif
	}
	return 0;
}

size_t randomname(char * s, char *ext)
{
	int i;
	char let[] = "0123456789abcdef";

	srand(time(0));
	for(i=0; i < 32; i++)
		s[i] = let[rand()%strlen(let)];

	/* copy the extension */
	s[i] = '.';
	s[i+1] = ext[0];s[i+2] = ext[1];
	s[i+3] = ext[2];s[i+4] = ext[3];
	s[i+5] = '\0';

	return 0;
}

void upload_error(char * str)
{
	sprintf(str);
	int fp = open_write("/tmp/upload.log");
	write(fp, str, strlen(str));
	close(fp);
}

int write_file(char *fname, char * data, size_t dlen)
{
	int fp;
	char path[128];

	strcpy(path, UPLOAD_PATH);
	strncat(fname, fname, (128-1)-strlen(UPLOAD_PATH));
	fp = open_write("/tmp/file");
	if(fp == -1)
		upload_error("Could not create new file");
	write(fp, data, dlen);
	close(fp);
	return 1;
}

int isvalid(char s)
{
	if(s != '\\' && s!= '\"')
		return 1;
	return 0;
}

size_t extract_fileext(char *fname, char *ps)
{
	char *begin,*end,*dot;
	int i;

	/* find the beginning */
	begin = strstr(ps, "filename");
	if(begin==NULL)
		return -1;

	/* find the end including value */
	end = strchr(begin, '\n');
	if(end==NULL)
		return -1;

	*end = '\0'; /* 1: end the string */

	/* find the last dot */
	dot = strrchr(begin, '.');
	if(dot == NULL)
		return -1;

	*end = '\n';/* 2: fix the string */

	/* copy the file extension */
	for(i=1;i<3;i++)
		if(!isvalid(dot[i]))
			return -1;

	fname[0] = dot[1];
	fname[1] = dot[2];
	fname[2] = dot[3];
	if(dot[4] != '\\' && dot[4] != '\"'){
		fname[3] = dot[4];
		return 4;
	}
	else
		fname[3] ='\0';

	return 3;
}

int main ()
{
	array ps;
	array content;
	size_t plen;
	int err;
	/* 16 random hex chars, 1 for the dot, 4 for the extension, 1 for \0 */
	char fname[128];
	char ext[4];

	memset(&content, 0, sizeof(array));
#if defined(WANT_FAST_CGI)
	while (FCGX_Accept(&fcgi_in, &fcgi_out, &fcgi_err, &envp) >= 0) {
#endif

		plen = get_post_string(&ps);
		if(plen >= UPLOAD_POST_LENGTH)
			upload_error("The file you are attempting to upload exceeds the maximum allowable file size (300kb)");

		err = extract_fileext(ext, ps.p);
		if(err == -1)
			upload_error("Could not extract file extension");

		do{
			randomname(fname, ext);
		}while (access(fname, F_OK) != -1);

		while (buffer_get_array(buffer_0, &content)) {}
		plen = array_bytes(&content);

		write_file(fname, content.p, plen);
		sprintmf("<a href=\"", UPLOAD_URL, "/", fname, "\">", UPLOAD_URL,"/" ,fname, "</a>");

#if defined(WANT_FAST_CGI)
	}
#endif
	return 0;
}
