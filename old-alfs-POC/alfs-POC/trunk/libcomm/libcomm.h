#ifndef __COMM_H__
#define __COMM_H__

#include <config.h>
#include <nc_glib.h>
#include <nc_sock.h>

#define SEC_LEN		128
#define MANGO		127

typedef void * (*reader_t) ();

extern sock cl;

int comm_deinit ();
int comm_init (char *hst, char *dr);
int comm_srv_deinit ();
int comm_srv_init (int argc, char **argv);

char comm_rdch ();
void *comm_rdchunk ();
float comm_rdfloat ();
int comm_rdint ();
char *comm_rdstr ();
GList *comm_rdlist (reader_t reader);

void def_str_writer (gpointer data);

int comm_wrch (char c);
int comm_wrchunk (void *data, int len);
int comm_wrfloat (float f);
int comm_wrint (int i);
int comm_wrstr (char *str);
int comm_wrlist (GList *list, GFunc writer, gpointer userdata);

#endif
