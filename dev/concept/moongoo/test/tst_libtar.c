#include <string.h>

#include <libalfs.h>
#include <tst.h>

int main (int argc, char **argv)
{
	char *host = popen_read("hostname");
	int ret;
	
	host = strcut(host, 0, strlen(host)-1);
	if (strcmp(host, "Lenin"))
		return 0;
	
	ret = tst_harness(directory("/usr/src/Packages/xvidcore-0.9.0.tar.bz2"), 
		"xvidcore-0.9.0/");
	ret += tst_harness(directory("/usr/src/Packages/zip23.tar.gz"),
		"zip-2.3/");
	return ret;
}
