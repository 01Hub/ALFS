#ifndef __MAIN_H
#define __MAIN_H

#define DEF_SYN		"book"
#define MOO_XML		"/.nALFS/answers.xml"
#define NAME		"alfs"
#define PLUG_DIR	"./"
#define PKG_XML		"packages.xml"
#define	VERSION		"0.0.4"

/* Some default settings, those should be configurable in the final tool */
role default_filter[4] = { NOEXECUTE, INTERACTIVE, TESTSUITE, 0 };
char *paralell_filter[4] = { "configure-host", "clean", "mrproper", NULL };
char *popt_pkg[2] = { "Glibc", NULL };
char *popt_cmd[2] = { "PARALLELMFLAGS=-j", NULL };

#endif
