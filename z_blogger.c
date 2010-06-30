#include <unistd.h>
#include <stdlib.h>

#include <array.h>
#include <str.h>
#include <taia.h>

#include "z_features.h"
#include "z_cdb.h"
#include "z_entry.h"
#include "z_blog.h"
#include "z_time.h"

#ifdef ADMIN_MODE

void read_stdin(array * content)
{
	while (buffer_get_array(buffer_0, content)) {
	}
}

void print_help()
{
	sprintf("Usage: cblog [OPTION]\n"
		"Reads value from stdin\n"

		"  -b path     set db file\n"
		"  -k key      key\n"
		"  -v value    value\n"

		"  -i          read value from stdin\n"

		"  -a          add\n"
		"  -d          delete\n"
		"  -m          modify\n"
		"  -s          show\n"

		"  -t          taia mode (for blog posts)\n"

		"  -h          print help\n");
}

/* options */
int main(int argc, char **argv)
{
	static struct nentry entry;
	char c;
	int err = 0;

	char db[32] = DB_FILE;
	struct cdb *cdb;

	char skey[100];
	static array value;
	char fmtkey[FMT_TAIA_HEX];

	enum mode { TAIA, PLAIN } mode = PLAIN;
	enum action {ADD, DEL, MOD, SHOW, ADD_NOW, HELP} action = HELP;
	/* parse */
	while ((c = getopt(argc, argv, "b:k:v:tandmsh?")) != -1) {
		switch (c) {
		case 'b':
			str_copy(db, optarg);
			break;
		case 'k':
			strcpy(skey, optarg);
			break;
		case 'v':
			array_cats0(&value, optarg);
			break;
		case 'i':
			read_stdin(&value);
			break;
		case 't':
			mode = TAIA;
			break;
		case 'a':
			action = ADD;
			break;
		case 'n':
			action = ADD_NOW;
			break;
		case 'd':
			action = DEL;
			break;
		case 'm':
			action = MOD;
			break;
		case 's':
			action = SHOW;
			break;
		default:
			action = HELP;
			break;
		}
	}

	if(mode == TAIA){
		if (strlen(skey) != 32) {
				sprintf("Key must by 32 hex characters wide\n");
				exit(2);
		}
		scan_time_hex(skey, &entry.k);
		if(array_bytes(&value)!=0)
			array_cats0(&entry.e, value.p);
	}

	switch(action){
	case ADD_NOW:
		err = add_entry_now(db, &entry);
		entry_dump(&entry);
		break;
	case ADD:
		if(mode == TAIA){
			err = add_entry(db, &entry);
			entry_dump(&entry);
		}else{
			cdb_add(db, skey, strlen(skey), value.p, strlen(value.p)+1);
		}
		break;
	case DEL:
		if(mode == TAIA){
			err = delete_entry(db, &entry);
		}else{
			err = cdb_del(db, skey, strlen(skey));
		}
		break;
	case SHOW:
		if(mode == TAIA){
			cdb= cdb_open_read(db);
			err = _show_entry(cdb, &entry);
			cdb_close(cdb);
			fmt_time_hex(fmtkey, &entry.k);
			if(err > 0)
				sprintmf(fmtkey, ":", entry.e.p, "\n");
			else
				sprintf("No entry found\n");
		}else{
			err = _cdb_get_value(db, skey, strlen(skey), &entry);
			if( err > 0)
				sprintmf(skey, ":", entry.e.p, "\n");
			else
				sprintf("No entry found\n");
		}
		break;
	case MOD:
		if(mode == TAIA){
			err = modify_entry(db, &entry);
			entry_dump(&entry);
		}else{
			cdb_mod(db, skey, strlen(skey), value.p, strlen(value.p)+1);
		}
		break;
	default:
		print_help();
		break;
	}

	return err;
}

#endif
