#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tst.h>

static int __tst_harness (char *result, char *reference, bool dbg)
{
	int ret;

	if (dbg)
		printf("_%s_\n_%s_\n", result, reference);
	ret=strcmp(result, reference);
	free(result);

	return ret;
}

int tst_harness (char *result, char *reference)
{
	return __tst_harness(result, reference, false);
}

int tst_harness_dbg (char *result, char *reference)
{
	return __tst_harness(result, reference, true);
}
