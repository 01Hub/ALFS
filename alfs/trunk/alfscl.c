#include <libcomm.h>
#include <plugin.h>
#include <remote.h>
#include <stdio.h>
#include <stdlib.h>

void print (gpointer data, gpointer user_data)
{
	/*char *str = (char *)data;
	printf("%s\n", str);*/
	plug *tst = (plug *)data;
	printf("%s - %s\n", tst->name, tst->path);
}

int main ()
{
	profile *prof;
	//GList *plugs;
	
	if (comm_init("127.0.0.1", getenv("PWD"))<0)
		return 1;

	/*plugs = comm_rdlist(1);
	g_list_foreach(plugs, print, NULL);*/
	
	comm_wrint(2);
	comm_wrstr("/home/neocool/projects/alfs/books/book/index.xml");
	prof = profile_reader();
	printf("%s\n", comm_rdstr());
	
	comm_wrint(0);
	comm_deinit();

	print_profile(*prof);
	
	return 0;
}
