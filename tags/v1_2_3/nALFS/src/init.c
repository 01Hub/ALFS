/*
 *  init.c - Initializing options.
 *
 *  Copyright (C) 2001-2003
 *
 *  Neven Has <haski@sezampro.yu>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "bufsize.h"
#include "options.h"
#include "utility.h"
#include "logging.h"
#include "nprint.h"

#include "init.h"


/*
 *
 *
 * Environment variables.
 *
 *
 */

static INLINE char *alloc_alfs_directory_name(void)
{
	char *alfs_dir;
	char *dir;


	dir = getenv(ALFS_VAR);

	/* Variable is set, use it. */
	if (dir != NULL && strlen(dir) > 0) {
		return xstrdup(dir);
	}

	dir = get_home_directory();

	if (Empty_string(dir)) {
		/* Can't get home directory, we'll try some
		 * environment variables for temporary directories. */
		dir = getenv("TMP");

		if (Empty_string(dir)) {
			dir = getenv("TEMP");
		}
	}

	if (dir != NULL && strlen(dir) > 0) {
		alfs_dir = xstrdup(dir);
		append_str(&alfs_dir, "/" ALFS_DIRECTORY_NAME);

	} else { /* Damn! Well, you'll use this, like it or not. */
		alfs_dir = xstrdup("/tmp/" ALFS_DIRECTORY_NAME);
	}

	return alfs_dir;
}

void read_env_variables()
{
	char *dir = alloc_alfs_directory_name();

	set_string_option(opt_alfs_directory, dir);

	xfree(dir);
}

/*
 *
 *
 * RC file.
 *
 *
 */

static INLINE int handle_set_line(const char *opt, const char *val)
{
	int ret = 0;

	switch (set_yet_unknown_option(opt, val)) {
		case OPTION_SET:
			break;

		case OPTION_UNKNOWN:
			Nprint_err("Option \"%s\" is unknown.", opt);
			ret = -1;
			break;

		case OPTION_INVALID_VALUE:
			ret = -1;
			break;
	}

	return ret;
}

static INLINE int get_next_token(char **s, char *buffer)
{
	int ch;
	int quote = 0;
	int pos = 0;


	SKIPWS(*s); /* Skip leading whitespace. */

	while ((ch = *(*s)++)) {
		if (isspace(ch) && !quote) {
			break;

		} else if (ch == '\'' || ch == '"') { /* Quote. */
			if (!quote) {
				quote = ch;
			} else {
				break;
			}

		} else if (ch == '\\') {
			if (!quote) {
				Nprint_warn(
				"Escape character found outside of quotes.");
				return -1;
			}
			buffer[pos++] = **s;
			++(*s);

		} else {
			buffer[pos++] = ch;
		}
	}

	buffer[pos] = '\0';

	if (pos == 0) {
		Nprint_warn("Empty token.");
		return -1;
	}

	return 0;
}

static int parse_rc_line(char *line)
{
	char *s = line;
	char command[strlen(line) + 1];


	SKIPWS(s); /* Skip leading whitespace. */

	if (*s == 0 || *s == '#') { /* Only whitespace, or comment. */
		return 0;
	}

	if (get_next_token(&s, command) != 0) {
		return -1;
	}

	if (strcmp(command, "set") == 0) {
		char opt[strlen(line) + 1];
		char val[strlen(line) + 1];


		if (get_next_token(&s, opt) != 0) {
			return -1;
		}

		if (get_next_token(&s, val) != 0) {
			return -1;
		}

		return handle_set_line(opt, val);
	}

	Nprint_err("Unknown command (%s).", command);

	return -1;
}

static INLINE void parse_rc_file(const char *rcfile)
{
	int line_num = 0;
	char line[MAX_RC_LINE_LEN];
	FILE *fp;


	/* Open it. */
	if ((fp = fopen(rcfile, "r")) == NULL) {
		if (errno == ENOENT) {
			Nprint_err("No configuration file to read (%s).",
				   rcfile);
		} else {
			Nprint_err("Unable to open rc file (%s): %s",
				   rcfile, strerror(errno));
		}
		return; /* Just don't read it then. */
	}

	while (fgets(line, sizeof line, fp)) {
		line_num++;

		if (! remove_new_line(line)) { /* No '\n'. */
			int c;

			while ((c = getc(fp)) != EOF && c != '\n')
				/* Skip in circle. */;

			Nprint_err("File %s, line %d too long, won't read it.",
				   rcfile, line_num);
		}

		if (parse_rc_line(line) != 0) {
			Nprint_err("File %s, error at line %d: %s",
				   rcfile, line_num, line);
		}
	}

	fclose(fp);
}

/*
 * Read the "system" configuration file, /etc/nALFSrc.
 */
void read_system_rc_file(void)
{
	char system_rc_file[] = "/etc/nALFSrc";
	struct stat st;

	if (!stat(system_rc_file, &st))
		parse_rc_file(system_rc_file);
}

/*
 * Read the "user" configuration file, ~/.nALFSrc.
 */
void read_user_rc_file(void)
{
	char *home_dir;
	char *rcfile;
	struct stat st;

	home_dir = get_home_directory();

	if (Empty_string(home_dir)) {
		Nprint_warn("Can't find your home directory.");
		return;
	}

	rcfile = xstrdup(home_dir);
	append_str(&rcfile, "/");
	append_str(&rcfile, RC_FILE_NAME);

	if (!stat(rcfile, &st))
		parse_rc_file(rcfile);

	xfree(rcfile);
}

/*
 *
 *
 * Command-line options.
 *
 *
 */

enum long_option {
	LONG_OPTION_DISPLAY_ALFS = CHAR_MAX + 1,
	LONG_OPTION_DISPLAY_DOCTYPE,
	LONG_OPTION_DISPLAY_COMMENTS,
	LONG_OPTION_TIMER_SHOWS_CURRENT,
	LONG_OPTION_TIMER_SHOWS_TOTAL,
	LONG_OPTION_SYSTEM_OUTPUT,
	LONG_OPTION_VERSION,
	LONG_OPTION_HELP,
	LONG_OPTION_GENERATE_STAMP,
	LONG_OPTION_RCFILE,
};


static void print_version(void)
{
	fprintf(stderr,
"\n"
PACKAGE_STRING ", " COPYRIGHT "\n"
"nALFS comes with ABSOLUTELY NO WARRANTY.\n"
"This is free software, and you are welcome to redistribute it\n"
"under certain conditions. See COPYING for more info.\n"
"\n"
"Any suggestions, ideas, bug reports, patches etc. are welcome and\n"
"can be sent to " EMAIL ".\n"
"\n"
"For more information about the program and the latest version, check out\n"
"its home page at " SITE ".\n\n");

	exit(EXIT_FAILURE);
}

static INLINE void print_help_and_exit(const char *prog)
{
	char *file = alloc_real_status_logfile_name();

	if (prog == NULL) {
		prog = PACKAGE_NAME;
	}

	printf(
"\n"
"Usage: %s [OPTION]... <PROFILE>...\n"
"\n"
"Options (default settings are within parentheses):\n"
"\n"
"    --display-alfs                Toggle displaying of <alfs> elements (%s).\n"
"    --display-doctype             Toggle displaying of doctype (%s).\n"
"    --display-comments            Toggle displaying of comments (%s).\n"
"    --display-system-output       Toggle displaying of system output (%s).\n"
"    --timer-shows-current         Timer will show current execution time.\n"
"    --timer-shows-total           Timer will show total execution time.\n"
"    -l, --log-status-window       Toggle logging of status window (%s).\n"
"    -L, --log-file <file>         Use <file> for logging (\"%s\").\n"
"    -B, --log-backend             Log backend (files, handler actions) (%s).\n"
"    -f, --log-newer-files         Log changed files using time stamps.\n"
"    -h, --log-handler-actions     Toggle logging of handler actions (%s).\n"
"    -i, --interactive             Toggle interactive mode (%s).\n"
"    -s, --start                   Start parsing the profiles immediately.\n"
"    -b, --base <dir>              Use <dir> as a start directory for logging\n"
"                                  installed files.\n"
"    -p, --prune <dirs>            Ignore <dirs> (separated with spaces) when\n"
"                                  logging files.\n"
"    -S, --generate-stamp          Toggle stamp mode.\n"
"    --rcfile <file>               Use <file> as configuration file.\n"
"    -v, --verbose                 Toggle verbosity (%s).\n"
"    --version                     Display program's version.\n"
"    --help                        Display this help.\n"
"\n",
	prog,
	*opt_display_alfs ? "on" : "off",
	*opt_display_doctype ? "on" : "off",
	*opt_display_comments ? "on" : "off",
	*opt_show_system_output ? "on" : "off",
	*opt_log_status_window ? "on" : "off",
	file,
	*opt_log_backend ? "on" : "off",
	*opt_log_handlers ? "on" : "off",
	*opt_run_interactive ? "on" : "off",
        *opt_be_verbose ? "on" : "off");

	xfree(file);

	exit(0);
}

static void print_usage_and_exit(void)
{
	fprintf(stderr,
		"Use --help for printing program's usage information.\n");

	exit(EXIT_FAILURE);
}

void read_command_line_options(int *argc, char ***argv)
{
	int i;
	struct option const long_opts[] = {

	{"display-alfs", no_argument, NULL, LONG_OPTION_DISPLAY_ALFS},
	{"display-doctype", no_argument, NULL, LONG_OPTION_DISPLAY_DOCTYPE},
	{"display-comments", no_argument, NULL, LONG_OPTION_DISPLAY_COMMENTS},
	{"display-system-output", no_argument, NULL, LONG_OPTION_SYSTEM_OUTPUT},
	{"timer-shows-current", no_argument, NULL, LONG_OPTION_TIMER_SHOWS_CURRENT},
	{"timer-shows-total", no_argument, NULL, LONG_OPTION_TIMER_SHOWS_TOTAL},
	{"log-status-window", no_argument, NULL, 'l'},
	{"log-backend", no_argument, NULL, 'B'},
	{"log-newer-files", no_argument, NULL, 'f'},
	{"log-handler-actions", no_argument, NULL, 'h'},

	{"interactive", no_argument, NULL, 'i'},
	{"start", no_argument, NULL, 's'},

	{"log-file", required_argument, NULL, 'L'},
	{"base", required_argument, NULL, 'b'},
	{"prune", required_argument, NULL, 'p'},

	{"verbose", no_argument, NULL, 'v'},

	{"version", no_argument, NULL, LONG_OPTION_VERSION},
	{"help", no_argument, NULL, LONG_OPTION_HELP},

	{"generate-stamp", no_argument, NULL, LONG_OPTION_GENERATE_STAMP},

	{"rcfile", required_argument, NULL, LONG_OPTION_RCFILE},

	{NULL, no_argument, NULL, 0}

	};


	while ((i = getopt_long(*argc, *argv, "lPfhisBSL:b:p:v", long_opts, NULL)) != -1) {
		switch (i) {
			case LONG_OPTION_DISPLAY_ALFS:
				Toggle(*opt_display_alfs);
				break;
			case LONG_OPTION_DISPLAY_DOCTYPE:
				Toggle(*opt_display_doctype);
				break;
			case LONG_OPTION_DISPLAY_COMMENTS:
				Toggle(*opt_display_comments);
				break;
			case LONG_OPTION_SYSTEM_OUTPUT:
				Toggle(*opt_show_system_output);
				break;

			case LONG_OPTION_TIMER_SHOWS_CURRENT:
				*opt_display_timer = TIMER_CURRENT;
				break;
			case LONG_OPTION_TIMER_SHOWS_TOTAL:
				*opt_display_timer = TIMER_TOTAL;
				break;

			case 'l':
				Toggle(*opt_log_status_window);
				break;
			case 'B':
				Toggle(*opt_log_backend);
				break;
			case 'f':
				*opt_logging_method = LOG_USING_ONE_FIND;
				break;
			case 'h':
				Toggle(*opt_log_handlers);
				break;

			case 'i':
				Toggle(*opt_run_interactive);
				break;
			case 's':
				*opt_start_immediately = 1;
				break;

			case 'L':
				set_string_option(opt_status_logfile, optarg);
				break;

			case 'b':
				set_string_option(opt_find_base, optarg);
				break;
			case 'p':
				append_string_option(opt_find_prunes, optarg);
				break;

			case 'v':
				Toggle(*opt_be_verbose);
				break;

			case LONG_OPTION_VERSION:
				print_version(); /* Function does exit(). */
				break;

			case 'S':
			case LONG_OPTION_GENERATE_STAMP:
				Toggle(*opt_stamp_packages);
				break;

			case LONG_OPTION_RCFILE:
				parse_rc_file(optarg);
				break;

			case LONG_OPTION_HELP:
				print_help_and_exit(*argv[0]);

			default:
				print_usage_and_exit();
		}
	}

	/* Check for specified profiles. */
	if (optind >= *argc) {
		Nprint_err("No profiles specified.");
		print_usage_and_exit();
	}
}

/*
 *
 *
 * Directories that nALFS uses.
 *
 *
 */

static void do_create_directory(const char *dir)
{
	if (mkdir(dir, DIRS_MODE) && errno != EEXIST) {
		fprintf(stderr, "Can't create \"%s\": %s\n",
			dir, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static INLINE void create_packages_directory(const char *dir)
{
	do_create_directory(dir);

	printf("Created packages directory (%s).\n", dir);
}

static INLINE void create_alfs_directory(void)
{
	int c;
	char input[4];


	printf(
"\n"
"nALFS needs a directory for storing various useful information - different\n"
"logs, information about installed packages, compile times etc.\n"
"\n"
"You can change the name of this directory in several ways, refer to\n"
"nALFSrc configuration file for more info.\n\n");

	do {
		printf("Do you want to create \"%s\" now (N/y) ? ",
			*opt_alfs_directory);
		fflush(stdout);

	} while (fgets(input, sizeof input, stdin) == NULL);

	if (! remove_new_line(input)) { /* No '\n'. */
		while ((c = getc(stdin)) != EOF && c != '\n')
			/* Skip in circle. */;
	}

	if (input[0] == 'y' || input[0] == 'Y') {
		do_create_directory(*opt_alfs_directory);

		printf("Directory \"%s\" created.\n\n", *opt_alfs_directory);
		printf("Press enter to continue. ");

		getchar();

	} else {
		printf("Directory not created.\n\n");
		exit(0);
	}
}

static int check_directory(const char *dir)
{
	struct stat dir_stat;


	if (dir[0] != '/') {
		fprintf(stderr, "Need absolute path, can't use %s.\n", dir);
		return -1;
	}

	if (stat(dir, &dir_stat)) {
		if (errno != ENOENT) {
			fprintf(stderr, "Can't get status for \"%s\": %s\n",
				dir, strerror(errno));
			return -1;
		}

		/* Directory doesn't exist. */

		return 1;
	}

	/* Check if it's a directory. */
	if (! S_ISDIR(dir_stat.st_mode)) {
		fprintf(stderr, "\"%s\" is not a directory.\n", dir);
		return -1;
	}

	/* Check for permissions. */
	if (access(dir, R_OK | W_OK | X_OK)) {
		fprintf(stderr, "You don't have the access to \"%s\": %s\n",
			dir, strerror(errno));
		return -1;
	}

	return 0;
}

void init_needed_directories(void)
{
	char *pdir;


	switch (check_directory(*opt_alfs_directory)) {
		case 0: /* Looking good. */
			break;

		case 1: /* It doesn't exist. Just create it. */
			create_alfs_directory();
			break;

		case -1: /* We can't use it. */
			exit(EXIT_FAILURE);
	}

       	pdir = alloc_real_packages_directory_name();

	switch (check_directory(pdir)) {
		case 0: /* Looking good. */
			break;

		case 1: /* It doesn't exist. Just create it. */
			create_packages_directory(pdir);
			break;

		case -1: /* We can't use it. */
			xfree(pdir);
			exit(EXIT_FAILURE);
	}

	xfree(pdir);
}
