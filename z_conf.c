#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <scan.h>
#include <array.h>
#include <str.h>
#include <open.h>

#include <openssl/sha.h>
#include "z_features.h"

#include "z_conf.h"
#include "z_cdbb.h"

#define CONF_DB "db/conf.inc"
#define SESSION_DB "db/session.inc"
#define SESSION_DB_TMP "db/session.tmp.inc"

int expire_all_sessions(blog_t *conf)
{
	int fd;
	unlink(SESSION_DB);
	fd = open_write(SESSION_DB);
	if(!fd)
		return -1;
	close(fd);
	return 0;
}

int load_config(blog_t * conf)
{
	int err;
	struct cdbb a;

	err = cdbb_open_read(&a, CONF_DB);
	if (err < 0)
		return -1;

	err = cdbb_readn(&a, "title", 5, conf->title, sizeof(conf->title));
	if(err)
		return -2;
	err = cdbb_readn(&a, "tagline", 7, conf->tagline, sizeof(conf->tagline));
	if(err)
		return -2;
	err = cdbb_readn(&a, "searchbox", 9, &conf->sbox, 1);
	if(err)
		return -2;

	cdbb_close_read(&a);

	return err;
}

#ifdef WANT_HTTP_304
int http_is_modified(char *lm, time_t t)
{
	time_t ims;
	scan_httpdate(lm,&ims);

	if(t>ims)
		return 1;
	return 0;
}

time_t http_if_modified_since(char *m)
{
	struct taia t;
	struct cdbb a;
	char pk[TAIA_PACK];
	int err;

	err = cdbb_open_read(&a, DB_FILE);
	if (err < 0)
		return -1;

	err = cdbb_readn(&a, "!lastmodified", 13, pk, TAIA_PACK);
	cdbb_close_read(&a);

	taia_unpack(pk, &t);

	if(err == 1 && m && !http_is_modified(m, (time_t)t.sec.x&0xffffffffffful)){
		return 0;
	}

	return (time_t)t.sec.x&0xffffffffffful;
}
#endif

#if defined(WANT_CGI_CONFIG) && defined(ADMIN_MODE)
/*  SESSION */

int is_expired(char * v)
{
		struct taia now, ts;
		taia_now(&now);

		taia_unpack((char*)v, &ts);

		if (taia_less(&now, &ts))
			return 0;
		return 1;
}

int filter_expired(unsigned char * k, size_t ks,
		unsigned char *v, size_t vs)
{
	return is_expired((char *)v);
}

int save_config(blog_t * conf)
{
	struct cdbb a;
	cdbb_start_mod(&a, CONF_DB);
	if(strcmp(conf->qry.title, "")){
		cdbb_rep(&a, "title", 5, conf->qry.title, strlen(conf->qry.title)+1);
		strcpy(conf->title, conf->qry.title);
	}
	if(strcmp(conf->qry.pass, ""))
		cdbb_rep(&a, "input", 5, conf->qry.pass, strlen(conf->qry.pass)+1);
	cdbb_rep(&a, "tagline", 7, conf->qry.tagline, strlen(conf->qry.tagline)+1);
	strcpy(conf->tagline, conf->qry.tagline);
#ifdef WANT_SEARCHING
	cdbb_rep(&a, "searchbox", 9, &conf->sbox, 1);
#endif
	cdbb_apply(&a);

	return 0;
}
#endif

#ifdef ADMIN_MODE_PASS
int auth_conf(blog_t * conf, unsigned char *in, size_t len)
{
	struct nentry * n = new_nentry();
	int auth;

	struct cdbb a;
	cdbb_open_read(&a, CONF_DB);
	if(cdbb_read_nentry(&a, "input", 5, n) <= 0){
		free_nentry(n);
		return 0;
	}
	cdbb_close_read(&a);

	auth = !memcmp(in, n->e.p, len);
	memset(n->e.p, 0, len);

	return auth;
};

/* generates a new session id, adds it to the db and returns it in buf
 * length is SESSION_ID_LEN */
int add_session_id(char *buf)
{
	int i, err;
	char letters[] = "0123456789abcdef";
	struct taia now;
	char ts[TAIA_PACK];
	struct cdbb a;

	srand((unsigned int)time(NULL));

	for (i = 0; i < SIZE_SESSION_ID; i++) {
		buf[i] = letters[(rand() % strlen(letters))];
	}

	/*  add value to sessiondb with timestamp */
	taia_now(&now);
	taia_addsec(&now, &now, SESSION_VTIME);
	taia_pack(ts, &now);

	err = cdbb_start_mod(&a, SESSION_DB);
	if(err)
		return -1;

	cdbb_add(&a, buf, SIZE_SESSION_ID, ts, TAIA_PACK);
	cdbb_add_filter(&a, filter_expired);
	cdbb_apply(&a);
	return 1;
}

/*  only call if cookie with session id is sent */
/*  0 means not validated, 1 means validated */
int validate_session_id(char *buf)
{
	int err;
	struct nentry value;
	struct cdbb a;

	memset(&value, 0, sizeof(struct nentry));
	cdbb_open_read(&a, SESSION_DB);
	err = cdbb_read_nentry(&a, buf, SIZE_SESSION_ID, &value);

	if (err <= 0){
		cdbb_close_read(&a);
		return 0;
	}

	return !is_expired(value.e.p);
}
#endif
