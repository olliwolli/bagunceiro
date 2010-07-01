void cdb_test()
{
	//int mod_add_str(const char * file, char * key, size_t ks, char *v){
	//
	//	int err;
	//
	//	struct cdb_action a = {
	//			.k = key,
	//			.ks = ks,
	//			.v  = v,
	//			.vs = strlen(v) + 1,
	//			.f = file
	//	};
	//
	//	__cdb_start_mod(a);
	//
	//	memset(&e, 0, sizeof(struct nentry));
	//	if(strcmp("", key)){
	//		err = _cdb_get(a.r, key, ks , &a.e);
	//
	//		if(err >= 1)
	//			__cdb_mod(a);
	//		else
	//			__cdb_add(a);
	//	}
	//	__cdb_finish_mod(a);
	//
	//	return 0;
	//}
	//
	//int save_config(blog_t * conf)
	//{
	//	struct cdb_action a = {
	//			.k = key,
	//			.ks = ks,
	//			.v  = v,
	//			.vs = strlen(v) + 1,
	//			.f = file,
	//			.needle = {"title", "input", "tagline"},
	//			.cbs = {__cdb_mod_add, __cdb_mod_add, __cdb_mod_add}
	//	};
	//
	//	__cdb_copy(a);
	//	a.
	//	__cdb_finish_mod(a);
	//	mod_add_str(CONF_DB , "title", 5, conf->qry.title);
	//	mod_add_str(CONF_DB , "input", 5, conf->qry.pass);
	//	mod_add_str(CONF_DB , "tagline", 7, conf->qry.tagline);
	//	return 0;
	//}

}

inline int __cdb_copy(struct cdb_action * a)
{
	uint32 kpos, kp, klen, dp, dlen;
	unsigned char *key, *data;
	int err, found, cb, i, gfound, copied;

	cb = CDB_DO_COPY;

	gfound = (a->cbsnum != 0) ? 0 : 1;

	if(!cdb_firstkey(a->r, &kpos))
		return 0;

	do{
		kp = cdb_keypos(a->r);
		klen = cdb_keylen(a->r);
		key = alloca(klen);
		cdb_read(a->r, key, klen, kp);

		dp = cdb_datapos(a->r);
		dlen = cdb_datalen(a->r);
		data = alloca(dlen);
		cdb_read(a->r, data, dlen, dp);

		/* call all callbacks */
		for(i=0; i < a->cbsnum; i++){
			if(a->cbs[i] != 0){
				cb = a->cbs[i](a, &copied);
				if (cb != CDB_DO_COPY){
					err += copied;
					found = gfound=  1;
				}
			}
		}

		if (found==0) {
			cdb_make_add(a->w, key, klen, data, dlen);
			err++;
		}

	} while (cdb_nextkey(a->r, &kpos) == 1);

	if(!gfound){
		errno = ENOKEY;
		err = -2;
	}

	return err;
}

int find_needle(struct cdb_action * a)
{
	int i;

	for(i=0; i< a->cbsnum; i++){
		if(a->needle[i] != NULL &&
				!memcmp(a->k, a->needle[i], a->ks))
			return i;
	}
	return -1;
}

int __cdb_cb_no_copy(struct cdb_action * a, int * c)
{
	if (find_needle(a))
		return CDB_DO_COPY;
	*c = 0;
	return CDB_DO_NOT_COPY;
}

int __cdb_cb_day_idx_add(struct cdb_action * a, int * c)
{
	static array fmt;
	do_day_index(&fmt, a->needle, TAIA_PACK);

	cdb_make_add(a->w, (unsigned char *)fmt.p,
			array_bytes(&fmt), a->needle, TAIA_PACK);

	*c = 1;
	return CDB_DO_COPY;
}

int __cdb_cb_day_idx_del(struct cdb_action * a, int * c)
{
	static array fmt;
	do_day_index(&fmt, a->needle, TAIA_PACK);
	if(find_needle(a)){
		return CDB_DO_NOT_COPY;
	}
	c = 0;
	return CDB_DO_COPY;
}

inline int __cdb_add(struct cdb_action * a)
{
	cdb_make_add(a->w, (unsigned char *)a->k, a->ks, (unsigned char *)a->v, a->vs);
	return 1;
}

/* elegant */
inline int __cdb_mod(struct cdb_action * a)
{
	int err;

	err = __cdb_copy(a);
	if(err == -2) /* key was not found */
		return err;
	err += __cdb_add(a);

	return err;
}

inline int __cdb_mod_add(struct cdb_action * a)
{
	int err;

	/* copy even if key was not found */
	__cdb_copy(a);
	err = __cdb_add(a);

	return err;
}

void __cdb_start_mod(struct cdb_action * a)
{
	memset(&a->e, 0, sizeof(struct nentry));
	strcpy(a->t, a->f);
	a->t = malloc(strlen(a->f) +5);
	strcat(a->t, ".tmp");

	a->w = cdb_open_write(a->t);
	a->r = cdb_open_read(a->f);
}

void __cdb_finish_mod(int err, struct cdb_action * a)
{
	rename(a->t, a->f);

	if (err == 0)
		unlink(a->f);

	if(a->t)
		free(a->t);
	cdb_close(a->r);
	cdbm_make_close(a->w);

}

/* search key in result and write result as array in v
 * NOTE: this if you have multiple values under the same hash
 * this functions assumes that these values have a fixed length
 * known to the caller (the data will be in newv->e)
 *  */
int _cdb_get(struct cdb_action * a)
{
	int err, dlen;

	unsigned char * buf;

	err = cdb_find(a->r, (unsigned char *)a->k, a->ks);

	if (err <= 0) {
		return err;
	} else {
		err = 0; /* num */
		/* set key */
		a->e.kp = malloc(a->ks);
		memcpy(a->e.kp, a->k, a->ks);
		taia_unpack(a->k, &a->e.k);

		do {
			dlen = cdb_datalen(a->r);

			buf = alloca(dlen);

			if(cdb_read(a->r,buf,dlen, cdb_datapos(a->r)) < 0)
					return -2;

			array_catb(&a->e.e, (char *)buf, dlen);
			err++;
		} while (cdb_findnext(a->r, (unsigned char *)a->k, a->ks) > 0);
	}
	return err;
}
