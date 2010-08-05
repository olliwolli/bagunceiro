
struct db_conf {
	char k[SIZE_HTTP_ARG];
	size_t max;		/* value max */
	char *v;
};

/* predefined database keys */
//#define CONF_TITLE "title"
//#define CONF_TAGLINE "tagline"
//#define CONF_SEARCHBOX "searchbox"
//#define CONF_PASS "input"

//#define POST_TITLE "title"
//#define POST_TAGLINE "tagline"
//#define POST_PASS "pass"
//#define POST_SEARCHBOX "sbox"
//#define POST_INPUT "input"
//#define POST_KEY "key"
//#define POST_LOGIN "login"
//#define POST_FILE_UPLOAD "file"
//#define POST_ADD "add"
//#define POST_MOD "mod"
//#define POST_CONFIG "config"
//#define POST_ACTION "action"

//#define COOKIE_SID "sid"
//#define COOKIE_CSS "css"

//#define DB_LAST_MODIFIED "!lastmodified"

enum e_conf {
	CONF_TITLE, CONF_TAGLINE, CONF_SEARCHBOX, CONF_PASS,

	DB_LAST_MODIFIED,

	POST_TITLE, POST_TAGLINE, POST_PASS, POST_SEARCHBOX,
	POST_INPUT, POST_KEY, POST_LOGIN, POST_FILE_UPLOAD,
	POST_ACTION,

	QUERY_QRY, QUERY_TAG, QUERY_TS, QUERY_DEL, QUERY_MOD,
	QUERY_ADD, QUERY_CONFIG, QUERY_CSS, QUERY_FMT,
	QUERY_MONTH, QUERY_LOGIN, QUERY_LOGOUT,

	COOKIE_CSS, COOKIE_SID,
};

#define MAX_KEY_LENGTH_STR 50
#define MAX_FMT_LENGTH 5
#define P_NO_VALUE -2

static struct db_conf conf_title[] = {
	/* configuration values */
	{"title", SIZE_HTTP_ARG},
	{"tagline", SIZE_TAGLINE},
	{"searchbox", SIZE_HTTP_ARG},
	{"input", SIZE_HTTP_ARG},

	/* database */
	{"!lastmodified", -1},

	/* post values */
	{"title", SIZE_HTTP_ARG},
	{"tagline", SIZE_TAGLINE},
	{"pass", SIZE_HTTP_ARG},
	{"sbox", SIZE_HTTP_ARG},
	{"input", -1},		// FIXME
	{"key", MAX_KEY_LENGTH_STR},	// FIXME
	{"login", 100},
	{"file", -1},
#define POST_ACTION_ADD "add"
#define POST_ACTION_MOD "mod"
#define POST_ACTION_CONFIG "config"
	{"action", 7},

	/* query values */
	{"qry", SIZE_FIND_STR},
	{"tag", SIZE_FIND_STR},
	{"ts", MAX_KEY_LENGTH_STR},	// FIXME MAX_KEY_LENGTH_STR = 50
	{"del", MAX_KEY_LENGTH_STR},
	{"mod", MAX_KEY_LENGTH_STR},
	{"add", P_NO_VALUE},
	{"config", P_NO_VALUE},
	{"css", SIZE_HTTP_ARG},
	{"fmt", MAX_FMT_LENGTH},
	{"mn", FMT_CALDATE},
	{"login", P_NO_VALUE},
	{"logout", P_NO_VALUE},

	/* cookie values */
	{"css", SIZE_HTTP_ARG},
	{"sid", SIZE_SESSION_ID + 1}

};
