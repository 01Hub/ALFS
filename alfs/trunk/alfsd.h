#ifndef __ALFSD_H__
#define __ALFSD_H__

#define PLUG_DIR	"./"

role default_filter[4] = { NOEXECUTE, INTERACTIVE, TESTSUITE, 0 };
char *paralell_filter[4] = { "configure-host", "clean", "mrproper", NULL };
char *popt_pkg[2] = { "Glibc", NULL };
char *popt_cmd[2] = { "PARALLELMFLAGS=-j", NULL };

#endif
