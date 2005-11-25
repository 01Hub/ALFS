#include <libcomm.h>
#include <openssl/rand.h>
#include <secret.h>
#include <ssl.h>

char *secret;

char *gen_secret ()
{
	unsigned char buf[SEC_LEN];

	if (RAND_bytes(buf, SEC_LEN))
	{
		int i;
		char *ret = (char *)malloc((SEC_LEN-1)*2);
		strcpy(ret, "");
		for (i=0;i<SEC_LEN-1;i++)
			sprintf(ret, "%s%02x", ret, buf[i]);
		return ret;
	}
	
	return "";
}

bool check_secret ()
{
	char *sec = (char *)malloc(SEC_LEN+1);
	if (SSL_read(ssl, sec, SEC_LEN)<SEC_LEN)
		return false;
	if (!memcmp(secret, sec, SEC_LEN))
		return true;
	return false;
}
