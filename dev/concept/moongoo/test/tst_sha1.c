#include <stdio.h>

#include <libalfs.h>
#include <tst.h>

int main (int argc, char **argv)
{
	char *sum = popen_read("sha1sum /etc/fstab");
	sum = strcut(sum, 0, whereis(sum, ' '));
	return tst_harness(sha1sum("/etc/fstab"), sum);
}
