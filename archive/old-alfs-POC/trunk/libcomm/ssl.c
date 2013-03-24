#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#include <libcomm.h>
#include <ssl.h>

#define CHK_NULL(x) if ((x)==NULL) return 1
#define CHK_ERR(err,s) if ((err)==-1) { perror(s); return 1; }
#define CHK_SSL(err) if ((err)==-1) { ERR_print_errors_fp(stderr); return 2; }

SSL	*ssl;
SSL_CTX *ctx;

static int passwd_cb (char *buf, int size, int rwflag, void *userdata)
{
	strcpy(buf, PASSWD);
	return strlen(PASSWD);
}
	
int ssl_init ()
{
	int err;
	SSL_METHOD *meth;

	SSL_load_error_strings();
	SSLeay_add_ssl_algorithms();
	meth = SSLv23_server_method();
	ctx = SSL_CTX_new (meth);
	if (!ctx) 
	{
		ERR_print_errors_fp(stderr);
		return 2;
	}
	
	SSL_CTX_set_default_passwd_cb(ctx, passwd_cb);

	if (SSL_CTX_use_certificate_file(ctx, CERTF, SSL_FILETYPE_PEM) <= 0) 
	{
		ERR_print_errors_fp(stderr);
		return 3;
	}
	if (SSL_CTX_use_PrivateKey_file(ctx, KEYF, SSL_FILETYPE_PEM) <= 0) 
	{
		ERR_print_errors_fp(stderr);
		return 4;
	}
	if (!SSL_CTX_check_private_key(ctx)) 
	{
		fprintf(stderr,"Private key does not match the certificate public key\n");
		return 5;
	}

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, cl.fd);
	err = SSL_accept(ssl);
	CHK_SSL(err);
	return 0;
}

int ssl_deinit ()
{
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}

int ssl_srv_init ()
{
	int err;
	SSL_METHOD *meth;

	SSLeay_add_ssl_algorithms();
	meth = SSLv2_client_method();
	SSL_load_error_strings();
	ctx = SSL_CTX_new (meth);                        
	CHK_NULL(ctx);
	CHK_SSL(err);

	ssl = SSL_new(ctx);
	CHK_NULL(ssl);
	SSL_set_fd(ssl, cl.fd);
	err = SSL_connect(ssl);
	CHK_SSL(err);
	return 0;
}

int ssl_srv_deinit ()
{
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}
