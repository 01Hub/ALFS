#include <libcomm.h>
#include <ssl.h>

int comm_wrch (char ch)
{
	return SSL_write(ssl, &ch, sizeof(char));
}

int comm_wrstr (char *str)
{
	return comm_wrchunk(str, strlen(str)+1);
}

int comm_wrint (int i)
{
	return SSL_write(ssl, &i, sizeof(int));
}

int comm_wrfloat (float f)
{
	return SSL_write(ssl, &f, sizeof(float));
}

int comm_wrchunk (void *data, int len)
{
	comm_wrint(len);
	SSL_write(ssl, data, len);
	return 0;
}

int comm_wrlist (GList *list, GFunc writer, gpointer userdata)
{
	comm_wrint(g_list_length(list));
	g_list_foreach(list, writer, userdata);
	return 0;
}

void def_str_writer (gpointer data)
{
	char *str = (char *)data;
	comm_wrstr(str);
}
