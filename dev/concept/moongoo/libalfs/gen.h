#ifndef __GEN_H__
#define __GEN_H__

#define NUM_COMPR	2

extern const char *compr[NUM_COMPR], *unpck[NUM_COMPR];

char *directory (char *tarball);
char *read_file (char *fname);
char *popen_read (char *cmd);

#endif
