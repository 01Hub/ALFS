#ifndef __COMM_H__
#define __COMM_H__

#include <nc_sock.h>

#define CHK_NULL(x) if ((x)==NULL) exit (1)
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); exit(1); }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); exit(2); }

#define SEC_LEN		128
#define MANGO		127

int comm_deinit ();
int comm_init (char *hst, char *dr);
int comm_srv_deinit ();
int comm_srv_init (int argc, char **argv);

char comm_rdch ();
int comm_wrch (char c);

float comm_rdfloat ();
int comm_wrfloat (float f);

int comm_rdint ();
int comm_wrint (int i);

char *comm_rdstr ();
int comm_wrstr (char *str);

#endif
