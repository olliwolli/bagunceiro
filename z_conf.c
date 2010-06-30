#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <array.h>
#include <str.h>

#include <openssl/sha.h>
#include "z_features.h"

#include "z_cdb.h"
#include "z_conf.h"

#define CONF_DB "db/conf.inc"
#define SESSION_DB "db/session.inc"
#define SESSION_DB_TMP "db/session.tmp.inc"

int load_config(blog_t * conf)
{
	int err;
	struct nentry title;
	err = _cdb_get_value(CONF_DB, "title", 5, &title);
	strncpy(conf->title, title.e.p, sizeof(conf->title));

	err = _cdb_get_value(CONF_DB, "tagline", 7, &title);
	strncpy(conf->tagline, title.e.p, sizeof(conf->tagline));

	return err;
}

#if defined(WANT_CGI_CONFIG) && defined(ADMIN_MODE)
int mod_add_str(const char * file, char * key, size_t ks, char *v){

	int err;
	struct nentry e;
	memset(&e, 0, sizeof(struct nentry));
	if(strcmp("", key)){
		err = _cdb_get_value(file, key, ks, &e);
		if(err >= 1)
			cdb_mod(file, key, ks, v, strlen(v)+1);
		else
			cdb_add(file, key, ks, v, strlen(v)+1);
	}

	return 0;
}

int save_config(blog_t * conf)
{
	mod_add_str(CONF_DB , "title", 5, conf->qry.title);
	mod_add_str(CONF_DB , "input", 5, conf->qry.pass);
	mod_add_str(CONF_DB , "tagline", 7, conf->qry.tagline);
}
#endif

#ifdef ADMIN_MODE_PASS
int auth_conf(blog_t * conf, unsigned char *in, size_t len)
{
	struct nentry n;
	int auth;

	if(_cdb_get_value(CONF_DB, "input", 5, &n) <= 0)
		return 0;

	auth = !memcmp(in, n.e.p, len);
	memset(n.e.p, 0, len);

	return auth;
};

/*  SESSION */
static inline int is_expired(char *ts1)
{
	struct taia now, ts;
	taia_now(&now);

	taia_unpack(ts1, &ts);

	if (taia_less(&now, &ts))
		return 0;
	return 1;
}

/* dirty... merge with z_cdb.c */
static void clear_invalid()
{
	struct cdb_make *cdbm;
	struct cdb *c;
	unsigned char *key, *data;
	uint32 kpos, kp, klen, dp, dlen;

	cdbm = cdb_open_write(SESSION_DB_TMP);
	c = cdb_open_read(SESSION_DB);

	cdb_firstkey(c, &kpos);

	do {
		kp = cdb_keypos(c);
		klen = cdb_keylen(c);
		key = alloca(klen);
		cdb_read(c, key, klen, kp);

		dp = cdb_datapos(c);
		dlen = cdb_datalen(c);
		data = alloca(dlen);
		cdb_read(c, data, dlen, dp);

		if (!is_expired((char *)data))
			cdb_make_add(cdbm, key, klen, data, dlen);

	} while (cdb_nextkey(c, &kpos) == 1);
	cdbm_make_close(cdbm);
	cdb_close(c);
	rename(SESSION_DB_TMP, SESSION_DB);
}
/* generates a new session id, adds it to the db and returns it in buf
 * length is SESSION_ID_LEN */
void add_session_id(char *buf)
{
	int i;
	char letters[] = "0123456789abcdef";
	struct taia now;
	char ts[TAIA_PACK];

	srand((unsigned int)time(NULL));

	for (i = 0; i < SESSION_ID_LEN; i++) {
		buf[i] = letters[(rand() % strlen(letters))];
	}

	/*  add value to sessiondb with timestamp */
	taia_now(&now);
	taia_addsec(&now, &now, SESSION_VTIME);
	taia_pack(ts, &now);

	cdb_add(SESSION_DB, buf, SESSION_ID_LEN, ts, TAIA_PACK);
//      clear_invalid();TODO : activate
	clear_invalid();
}

/*  only call if cookie with session id is sent */
/*  TODO remove dead sessions */
/*  0 means not validated, 1 means validated */
int validate_session_id(char *buf)
{
	int err;
	struct nentry value;

	err = _cdb_get_value(SESSION_DB, buf, SESSION_ID_LEN, &value);

	if (err <= 0)
		return 0;

	if (!is_expired(value.e.p))
		return 1;
	return 0;
}

#endif
