#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/md5.h>
#include <openssl/sha.h>

#include <alfs.h>
#include <crypto.h>
#include <gen.h>

static char *__hashsum (char *fname, hash_algo algo)
{
	char *ret;
	unsigned char *digest, *data;
	unsigned long data_len, i, len;

	data = read_file(fname);
	data_len = strlen(data);
	switch (algo)
	{
		case ALGO_MD5:
			len = MD5_DIGEST_LENGTH;
			digest = (char *)malloc(len+1);
			MD5(data, data_len, digest);
			break;
		case ALGO_SHA1:
			len = SHA_DIGEST_LENGTH;
			digest = (char *)malloc(len+1);
			SHA1(data, data_len, digest);
			break;
		default:
			fprintf(stderr, "Unknown hash algorithm.\n");
			return NULL;
	}
	ret = (char *)malloc(42);
	strcpy(ret, "");
	for (i=0;i<len;i++)
		sprintf(ret, "%s%02x", ret, digest[i]);
	return ret;
}

char *md5sum (char *fname)
{
	return __hashsum(fname, ALGO_MD5);
}

char *sha1sum (char *fname)
{
	return __hashsum(fname, ALGO_SHA1);
}
