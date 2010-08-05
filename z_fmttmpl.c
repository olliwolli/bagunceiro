/* Make a template engine that does text replacement int html templates */
#include <open.h>

#define TEMPLATE "log.ct"

#define FILE_BUFFER_SIZE 8192
#define MAX_LINE_LENGTH 1024

enum pstate { INIT, LOOP } state = INIT;
enum pmatch { NONE, LOOP_BEGIN, LOOP_END, INCLUDE };

#define STR_LB "{=loop_begin=}"
#define STR_LE "{=loop_end=}"
#define STR_INCL "{=file_include=}"

int format_template(const blog_t * conf)
{
	static buffer tbuffer;
	static char tbuf[FILE_BUFFER_SIZE];
	char line[MAX_LINE_LENGTH];
	int fd, len;
	enum pmatch match = NONE;

	/* open file */
	fd = open_read(TEMPLATE);

	if (fd == -1)
		return -1;

	buffer_init(&tbuffer, read, fd, tbuf, sizeof tbuf);
	if ((len = buffer_getline(tbuffer, line, sizeof(line)))) {
		if (line[len] != '\n')
			return -1;	// line too long
		line[len] = 0;
		match = match_line(line);
		state_machine(line, match);
	}
}

enum pmatch match_line(char *s)
{
	if (strcmp(s, STR_LB))
		return LOOP_BEGIN;
	else if (strcmp(s, STR_LE))
		return LOOP_END;
	else if (strcmp(s, STR_INCL))
		return INCLUDE;
	else
		return NONE;
}

int state_machine(char *line, enum pmatch match)
{

}
