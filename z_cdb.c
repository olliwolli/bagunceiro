#include <unistd.h>
#include <stdio.h>

#include <cdb_make.h>
#include <cdb.h>
#include <open.h>
#include <byte.h>
#include <alloca.h>
#include <array.h>
#include <errno.h>

#include "z_cdb.h"
#include "z_entry.h"
#include "z_features.h"

typedef enum cop {
	OP_CPY, OP_MOD, OP_DEL, OP_ADD
}cop_t;

#define OP_CPY 0x00
#define OP_MOD 0x01
#define OP_DEL 0x02
#define OP_ADD 0x03
#define CP_NOP 0x00
#define CP_PAR 0x01
#define CP_SRC 0x02

static int exists_c(const char * file)
{
	if (access(file, F_OK | W_OK) != -1){
		return 1;
	}
	return 0;
}

static int exists(array * file)
{
	return exists_c(file->p);
}

static size_t __make_tmp_name(array * tmpname, const array * file)
{
	array_cats(tmpname, file->p);
	array_cats0(tmpname, ".tmp");
	return 0;
}

int cdb_init_file(array * file)
{
	int fp;
	struct cdb_make cdbm;

	if (exists(file)) {
		return 0;
	} else {
		fp = open_write(file->p);
		cdb_make_start(&cdbm, fp);
		cdb_make_finish(&cdbm);
		close(fp);
		return 1;
	}
}

/* dirty */
static int __cdb_remake_real(const char *name, const char *newname,
			     const unsigned char *skey, const
			     size_t slen,
			     unsigned char *value, size_t vlen, enum cop op)
{
	struct cdb_make cdbm;
	int numrec = 0;
	int first, loop_copy_src, new, old;
	struct cdb c;
	uint32 kpos;
	uint32 kp, klen, dp, dlen;
	unsigned char *key, *data;
	int found = 0;

	new = open_write(newname);
	if(new == -1){
		exists_c(newname);
	}
	old = open_read(name);
	if(old == -1){
		exists_c(name);
		return -1;
	}

	cdb_init(&c, old);
	first = cdb_firstkey(&c, &kpos);
	cdb_make_start(&cdbm, new);

	if (OP_ADD == op) {
		cdb_make_add(&cdbm, skey, slen, value, vlen);
		++numrec;

		if(first == -1 || first == 0)
			goto finish;

	}

	if (!first)
		return 0;
	do {
		loop_copy_src = CP_SRC;

		kp = cdb_keypos(&c);
		klen = cdb_keylen(&c);

		key = alloca(klen);

		cdb_read(&c, key, klen, kp);

		/* if the key is the same modifiy the entry */
		switch (op) {
		case OP_MOD:
			if (!byte_diff
			    ((const char *)key, klen, (const char *)skey)){
				found = 1;
				loop_copy_src = CP_PAR;
			}

			break;
		case OP_DEL:
			if (!byte_diff
			    ((const char *)key, klen, (const char *)skey)) {
				loop_copy_src = CP_NOP;
				found = 1;
			}
			break;
		case OP_CPY:
			/* fall-through */
		default:
			break;
		}

		switch (loop_copy_src) {
		case CP_PAR:
			data = value;
			dlen = strlen((const char *)value);
			cdb_make_add(&cdbm, key, klen, data, dlen);
			++numrec;
			break;
		case CP_NOP:
			break;
		case CP_SRC:
			/*  fall-through */
		default:
			++numrec;
			/* by default copy from src */
			dp = cdb_datapos(&c);
			dlen = cdb_datalen(&c);
			data = alloca(dlen);
			cdb_read(&c, data, dlen, dp);
			cdb_make_add(&cdbm, key, klen, data, dlen);
		}
	} while (cdb_nextkey(&c, &kpos) == 1);

finish:
	cdb_make_finish(&cdbm);
	close(c.fd);
	close(cdbm.fd);

	if( op == OP_DEL || op == OP_MOD)
		if (!found){
			errno = ENOKEY;
			return -2;
		}

	return numrec;
}

static int __cdb_remake(const array * file, const array * k, const array * v, enum cop op)
{
	static array tmpfile;
	int err;

	__make_tmp_name(&tmpfile, file);

	if(v!=NULL){
	err = __cdb_remake_real(file->p, tmpfile.p,
			  (unsigned char * )k->p, array_bytes(k), (unsigned char *) v->p, array_bytes(v), op);
	}else{
		err = __cdb_remake_real(file->p, tmpfile.p,
					  (unsigned char * )k->p, array_bytes(k), NULL, 0, op);
	}


	rename(tmpfile.p, file->p);
	if( err == 0){
		unlink(file->p);
	}
	array_reset(&tmpfile);
	return err;
}

int cdb_get(const array * file, const array * k, array * v)
{
	struct cdb result;
	int err, fd, len;

	unsigned char buf[MAX_ENTRY_SIZE];

	fd = open_read(file->p);
	if (fd <= 0) {
		return -1;
	}

	cdb_init(&result, fd);
	err = cdb_find(&result, (unsigned char * )k->p, array_bytes(k));

	if (err == 0) {
		return 0;
	} else if (err == -1) {
		return -1;
	} else {
		len = cdb_datalen(&result);
		err = cdb_read(&result, buf, len, cdb_datapos(&result));
		if (err < 0) {
			return -2;
		}
		array_allocate(v, 1, len + 1);
		return 1;
	}
}

int cdb_mod(const array * file, const array * k, const array * v)
{
	return __cdb_remake(file, k, v, OP_MOD);
}

int cdb_add(const array * file, const array * k, const array * v)
{
	return __cdb_remake(file, k, v, OP_ADD);
}

int cdb_del(const array * file, const array * k)
{
	return __cdb_remake(file, k, NULL, OP_DEL);
}

int cdb_read_all(const char *name, array * entries, struct eops *ops)
{
	int numdump = 0;
	int first;
	struct cdb c = {0};
	uint32 kpos;
	void *tmp;

	uint32 kp, klen, dp, dlen;
	unsigned char *key, *data;

	int fd = open_read(name);
	if (fd <= 0) {
		return -1;
	}
	cdb_init(&c, fd);
	first = cdb_firstkey(&c, &kpos);
	if (!first)
		return 0;
	do {
		kp = cdb_keypos(&c);
		klen = cdb_keylen(&c);
		dp = cdb_datapos(&c);
		dlen = cdb_datalen(&c);

		key = alloca(klen);
		data = alloca(dlen + 4);
		cdb_read(&c, key, klen, kp);
		cdb_read(&c, data, dlen, dp);

		/* key not found */
		if (klen == 0){
			return -2;
		}

		/* generic way */
		tmp = ops->alloc();
		ops->add_val(tmp, data, dlen);
		ops->add_key(tmp, key, klen);
		ops->add_to_array(tmp, entries);
		++numdump;
	} while (cdb_nextkey(&c, &kpos) == 1);
	close(fd);
	cdb_free(&c);
	return numdump;
}
