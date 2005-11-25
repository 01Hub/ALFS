#include <libcomm.h>
#include <nc_glib.h>
#include <ssl.h>

char comm_rdch ()
{	
	char ret;
	SSL_read(ssl, &ret, sizeof(char));
	return ret;
}

char *comm_rdstr ()
{
	return (char *)comm_rdchunk();
}

int comm_rdint ()
{
	int ret;
	SSL_read(ssl, &ret, sizeof(int));
	return ret;
}

float comm_rdfloat ()
{
	float ret;
	SSL_read(ssl, &ret, sizeof(float));
	return ret;
}

void *comm_rdchunk ()
{
	int len=comm_rdint();
	void *data=(void *)malloc(len+1);
	SSL_read(ssl, data, len);
	return data;
}

GList *comm_rdlist (reader_t reader)
{
	GList *ret=NULL;
	int i, len;

	len=comm_rdint();
	for (i=0;i<len;i++)
		ret=g_list_append(ret, reader());
	return ret;
}
