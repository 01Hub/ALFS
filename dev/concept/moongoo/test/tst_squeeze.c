#include <util.h>
#include <tst.h>

int main (int argc, char **argv)
{
	return tst_harness(squeeze("   moo  moongoo   "), "moo  moongoo");
}
