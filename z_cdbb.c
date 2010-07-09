#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <uint32.h>
#include <time.h>
#include <array.h>
#include <str.h>
#include <open.h>
#include <cdb_make.h>

#include "z_entry.h"
#include "z_blog.h"
#include "z_cdbb.h"
#include "z_features.h"


void init_nentry(struct nentry *n)
{
	memset(&n->e, 0, sizeof(array));
	memset(&n->k, 0, sizeof(array));
}

struct nentry *new_nentry(void)
{
	struct nentry *n;

	n = malloc(sizeof(struct nentry));
	init_nentry(n);

	return n;
}

void reset_nentry(struct nentry *n)
{
	array_reset(&n->e);
	array_reset(&n->k);
}

void free_nentry(struct nentry *n)
{
	reset_nentry(n);
	free(n);
}

/* OPERATIONS / FILTER SETUP */
/* LEVEL 0 - FILE */
int cdbb_open_read(struct cdbb *a, const char *f)
{
	int fd = open_read(f);
	if(fd <= 0)
		return fd;

	a->r = malloc(sizeof(struct cdb ));

	if(a->r == NULL){
		close(fd);
		return -1;
	}
	 /* suppress valgrind warning:
	  * "Uninitialised value was created by a heap allocation" in cdb_free */
	a->r->map = 0;

	cdb_init(a->r, fd);
	return fd;
}

void cdbb_close_read(struct cdbb *a)
{
	if (a->r->fd > 0)
		close(a->r->fd);
	cdb_free(a->r);
	free(a->r);
}

#ifdef ADMIN_MODE
int cdbb_open_write(struct cdbb *a, const char *f)
{
	int fp;

	unlink(f);
	fp = open_write(f);

	if(fp <= 0)
		return fp;

	a->w = malloc(sizeof(struct cdb_make));
	if(a->w == NULL){
		close(fp);
		return -1;
	}

	cdb_make_start(a->w, fp);
	a->fnum = 0;
	a->tnum = 0;
	return fp;
}

void cdbb_close_write(struct cdbb *a)
{
	cdb_make_finish(a->w);
	free(a->w);
}


/* LEVLEL COPYING LOGIC */
int cdbb_check_ops(struct cdbb *a, unsigned char *k,
		size_t ks, unsigned char * v, size_t vs)
{
	struct op * t;
	int i, f;
	f = 0;

	for (i = 0; i < a->tnum; i++) {
		t = &a->ops[i];
		if(array_bytes(&t->n) == ks && !memcmp(k, t->n.p, ks) && ks){
			t->kf = 1;

			if(t->t != O_ADD)
				f = 1;

			/* no value trigger */
			if((array_bytes(&t->v) != 0)){
				if(t->t == O_MOD || t->t == O_ADD_MOD)
					continue;

				/* value trigger */
				t->sv = realloc(t->sv,vs);
				if(t->sv == NULL)
					continue;

				memcpy(t->sv, v, vs);

				/* values do match */
				if ( !memcmp(t->v.p, t->sv, vs))
					t->vf = 1;
				else
					f = 0; /* reset found */
			}
		}
	}

	return f;
}

/* LEVLEL COPYING LOGIC */
int cdbb_check_filter(struct cdbb *a, unsigned char *k, size_t ks, unsigned char * v, size_t vs)
{
	int i;
	for(i=0; i < a->fnum; i++){
		if(a->fil[i](k, ks, v, vs))
			return 1;
	}
	return 0;
}

int cdbb_handle_ops(struct cdbb *a)
{
	int i;
	int num;
	struct op *t;

	num = 0;

	for(i=0; i < a->tnum; i++){
		t = &a->ops[i];

		if(t->kf){ /* trigger was fired */
			switch(t->t){
			case O_ADD:
			case O_ADD_MOD:
				/* fall-trought */
			case O_MOD:
				cdb_make_add(a->w,
							(unsigned char * )t->n.p, array_bytes(&t->n),
							(unsigned char * )t->v.p, array_bytes(&t->v));
				num++;
				break;
			case O_DEL:
				/* do nothing, fall-trough */
			case O_NONE:
			default:
				break;
			}
		}else{
			switch(t->t){
			case O_ADD_MOD:
				/* key not found so add, fall-trough */
			case O_ADD:
				cdb_make_add(a->w,
						(unsigned char * )t->n.p, array_bytes(&t->n),
						(unsigned char * )t->v.p, array_bytes(&t->v));
				num++;
				break;
			case O_DEL:
				/* key not found error */
				break;
			case O_MOD:
				/* key not found error */
				break;
			case O_NONE:
				/* fall-trought */
			default:
				break;
			}
		}
	}
	return num;
}

int __cdbb_copy(struct cdbb * a, size_t ks, size_t vs)
{
	unsigned char k[ks];
	unsigned char v[vs];
	uint32 kp, dp;
	int found, filter;

	found = 0;

	kp = cdb_keypos(a->r);
	cdb_read(a->r, k, ks, kp);

	dp = cdb_datapos(a->r);
	/* in some cases we do not need to read e.g. when deleting
	 * or filtering but for simplicity we just do it anyways */
	cdb_read(a->r, v, vs, dp);

	found = cdbb_check_ops(a, k, ks, v, vs);
	filter = cdbb_check_filter(a, k, ks, v, vs);

	/* if an operation applied or a filter, don't copy */
	if(!found && !filter && kp){
		cdb_make_add(a->w, k, ks, v, vs);
		return 1;
	}
	return 0;
}

int cdbb_copy(struct cdbb * a)
{
	uint32 kfindpos,ks, vs;
	int num = 0;

	if (!cdb_firstkey(a->r, &kfindpos))
		return 0;

	do {
		ks = cdb_keylen(a->r);
		vs = cdb_datalen(a->r);
		num += __cdbb_copy(a, ks, vs);
	} while (cdb_nextkey(a->r, &kfindpos) == 1);

	return num;
}


/* LEVEL 1 - SETUP */
void cdbb_add_filter(struct cdbb *a, filter cb)
{
	a->fil[a->fnum++] = cb;
}

void cdbb_add_op(struct cdbb *a, char * n, size_t ns, enum opt t)
{
	array * c= &a->ops[a->tnum].n;
	memset(&a->ops[a->tnum].v, 0, sizeof(array));
	a->ops[a->tnum].svs = 0;
	a->ops[a->tnum].sv = NULL;
	a->ops[a->tnum].t = t;
	memset(c, 0, sizeof(array));
	array_catb(c, n, ns);

	++(a->tnum);
}

void cdbb_add_vop(struct cdbb *a, char * n,
		size_t ns, char *v, size_t vs, enum opt t)
{
	cdbb_add_op(a, n, ns, t);
	/* write to tnum -1 because add_op already incremented */
	array_catb(&a->ops[a->tnum-1].v, v, vs);
}

void free_ops(struct cdbb * a)
{
	int i;
	for(i=0; i < a->tnum; i++){
		if(a->ops[i].sv)
			free(a->ops[i].sv);
		array_reset(&a->ops[i].n);
		array_reset(&a->ops[i].v);
	}
}

int cdbb_start_mod(struct cdbb * a, char * f)
{
	int err;
	a->f = f;
	a->t = malloc(strlen(a->f)+5);
	if(a->t == NULL)
		return -1;

	strcpy(a->t, a->f);
	strcat(a->t, ".tmp");

	err = cdbb_open_write(a, a->t);
	if(err <= 0)
		return err;

	err = cdbb_open_read(a, a->f);
	return err;
}

int cdbb_apply(struct cdbb * a) {
	int err = 0;

	err= cdbb_copy(a);

	err += cdbb_handle_ops(a);

	rename(a->t, a->f);

	if (err == 0)
		unlink(a->f);

	if (a->t)
		free(a->t);
	cdbb_close_read(a);
	cdbb_close_write(a);
	free_ops(a);
	return err;
}

/* HIGH LEVEL OPERATIONS, let the compiler do inlining, clang seems to have a bug when
 * using explicit inlining*/
void cdbb_add(struct cdbb *a, char *k, size_t ks, char *v, size_t vs)
{
	cdbb_add_vop(a, k, ks, v, vs, O_ADD);
}

void cdbb_mod(struct cdbb *a, char *k, size_t ks, char *v, size_t vs)
{
	cdbb_add_vop(a, k, ks, v, vs, O_MOD);
}

void cdbb_rep(struct cdbb *a, char *k, size_t ks, char *v, size_t vs)
{
	cdbb_add_vop(a, k, ks, v, vs, O_ADD_MOD);
}

void cdbb_del(struct cdbb *a, char *k, size_t ks)
{
	cdbb_add_op(a, k, ks, O_DEL);
}

void cdbb_del_val(struct cdbb *a, char *k, size_t ks, char *v, size_t vs)
{
	cdbb_add_vop(a, k, ks, v, vs, O_DEL);
}

#endif
int cdbb_readn(struct cdbb *a, char *k, size_t ks, char *v, size_t n)
{
	int dlen;

	if(cdb_find(a->r, (unsigned char *) k,  ks) <= 0)
		return -1;

	dlen = cdb_datalen(a->r);

	if(dlen > n)
			return -2;

	if (cdb_read(a->r, (unsigned char * )v, dlen, cdb_datapos(a->r)) < 0)
		return -3;

	return 1;
}

int cdbb_firstkey(struct cdbb * a)
{
	if (!cdb_firstkey(a->r, &a->kfindpos))
		return -1;

	return 1;
}

int cdbb_findnext(struct cdbb * a, const char *needle, struct nentry *n){
	uint32 kp, dp, ks, vs;
	unsigned char *k, *v;

	if(cdb_nextkey(a->r, &a->kfindpos) != 1)
		return -1;

	kp = cdb_keypos(a->r);
	ks = cdb_keylen(a->r);
	k = alloca(ks);
	cdb_read(a->r, k, ks, kp);

	dp = cdb_datapos(a->r);
	vs = cdb_datalen(a->r);
	/* in some cases we do not need to read e.g. when deleting
	 * or filtering but for simplicity we just do it anyways */
	v = alloca(vs);
	cdb_read(a->r, v, vs, dp);

	/* search value */
	if(strstr((char *)v,needle) == NULL)
		return 0;

	array_catb(&n->k, (char *)k, ks);
	array_catb(&n->e, (char *)v, vs);

	return 1;
}

int cdbb_read_nentry(struct cdbb *a, char *k, size_t ks, struct nentry *n)
{
	int err, dlen;
	unsigned char * buf;

	array_catb(&n->k, k, ks);
	err = cdb_find(a->r, (unsigned char *) k,  ks);

	if (err <= 0)
		return err;

	err = 0; /* num */
	do {
		dlen = cdb_datalen(a->r);
		buf = alloca(dlen);

		if (cdb_read(a->r, buf, dlen, cdb_datapos(a->r)) < 0)
			return -2;

		array_catb(&n->e, (char *) buf, dlen);
		err++;
	} while (cdb_findnext(a->r, (unsigned char *) k, ks) > 0);
	return err;
}
