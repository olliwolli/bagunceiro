#include <unistd.h>
#include <stdlib.h>

#include <array.h>
#include <str.h>
#include <taia.h>
#include <textcode.h>

#undef WANT_FAST_CGI
#include "z_features.h"
#include "z_cdbb.h"
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
		"  -n          add now \n"
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
	struct cdbb a;

	struct taia t;

	char skey[100];
	static array value;
	char fmtkey[FMT_TAIA_HEX];

	enum mode { TAIA, PLAIN } mode = PLAIN;
	enum action { ADD, DEL, MOD, SHOW, ADD_NOW, HELP } action = HELP;
	/* parse */
	while ((c = getopt(argc, argv, "b:k:v:tandmshi?")) != -1) {
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

	if (mode == TAIA) {
		if (strlen(skey) != 32 && action != ADD_NOW) {
			sprintf("Key must by 32 hex characters wide\n");
			exit(2);
		} else {
			scan_time_hex(skey, &t);
		}

		if (array_bytes(&value) != 0)
			array_cats0(&entry.e, value.p);
	}

	switch (action) {
	case ADD_NOW:
		cdbb_start_mod(&a, db);
		add_entry_now(&a, entry.e.p, array_bytes(&entry.e));
		err = cdbb_apply(&a);
		break;
	case ADD:
		cdbb_start_mod(&a, db);
		if (mode == TAIA) {
			add_entry(&a, &t, entry.e.p, array_bytes(&entry.e));
		} else
			cdbb_add(&a, skey, strlen(skey), value.p,
				array_bytes(&value));

		err = cdbb_apply(&a);
		break;
	case DEL:
		cdbb_start_mod(&a, db);
		if (mode == TAIA)
			delete_entry(&a, &t);
		else
			cdbb_del(&a, skey, strlen(skey));
		err = cdbb_apply(&a);
		break;
	case SHOW:
		cdbb_open_read(&a, db);
		if (mode == TAIA) {
			err = show_entry(&a, &t, &entry);
			fmtkey[fmt_hexdump(fmtkey, entry.k.p, TAIA_PACK)] = 0;
			if (err > 0)
				sprintmf(fmtkey, ":", entry.e.p, "\n");
		} else {
			err = cdbb_read_nentry(&a, skey, strlen(skey), &entry);
			if (err > 0)
				sprintmf(skey, ":", entry.e.p, "\n");
		}
		if (err <= 0)
			sprintf("No entry found\n");
		cdbb_close_read(&a);
		break;
	case MOD:
		cdbb_start_mod(&a, db);
		if (mode == TAIA) {
			modify_entry(&a, &t, value.p, array_bytes(&value));
		} else {
			cdbb_mod(&a, skey, strlen(skey), value.p,
				strlen(value.p) + 1);
		}
		err = cdbb_apply(&a);
		break;
	default:

		print_help();
		break;
	}

	return err;
}

#endif
