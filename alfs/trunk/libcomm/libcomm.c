#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <config.h>
#include <libcomm.h>

typedef struct
{
	char *host, *dir;
	int port;
} ssh_info;

static bool check_secret ();
static char *gen_secret ();
static int ssl_init();
static int ssl_deinit();
static int ssl_srv_init();
static int ssl_srv_deinit();
	
char *secret;
sock srv, cl;
SSL	*ssl;
SSL_CTX *ctx;

static void *spawn_srv (void *inf)
{
	char cmd[512];
	FILE *conn;
	ssh_info *info = (ssh_info *)inf;

	secret = gen_secret();
	snprintf(cmd, 512, "ssh %s \"cd %s;./alfsd %s %i %s\"", info->host, 
		info->dir, info->host, info->port, secret);
	conn = popen(cmd, "r");
	if (!conn)
		return NULL;
	while (fgets(cmd, 512, conn))
		printf("%s", cmd);
	pclose(conn);
	return NULL;
}

int comm_init (char *host, char *dir)
{
	pthread_t thr;
	ssh_info info = { host: host, dir: dir, port: 0 };
	
	sock_open(&srv, "127.0.0.1", 0);
	if (sock_bind(&srv))
		return -1;

	sock_listen(&srv, 1);
	info.port=ntohs(srv.in_adr.sin_port);
	pthread_create(&thr, NULL, spawn_srv, &info);
	sock_accept(&srv, &cl);
	ssl_init();
	if (!check_secret())
		return -1;
	return 0;
}

int comm_deinit ()
{
	while (comm_rdch()!=MANGO);
	sock_close(&cl, false);
	sock_close(&srv, false);
	ssl_deinit();
	return 0;
}

int comm_wrch (char ch)
{
	return SSL_write(ssl, &ch, sizeof(char));
}

char comm_rdch ()
{	
	char ret;
	SSL_read(ssl, &ret, sizeof(char));
	return ret;
}

int comm_wrstr (char *str)
{
	return SSL_write(ssl, str, strlen(str))+
			comm_wrch(0);
}

char *comm_rdstr ()
{
	int len=0;
	char c, *str=NULL;
	
	while ((c=comm_rdch())!=0)
	{
		str=realloc(str, (++len)*sizeof(char));
		str[len-1]=c;
	}
	
	str=realloc(str, (++len)*sizeof(char));
	str[len-1]='\0';
	return str;
}

int comm_wrint (int i)
{
	return SSL_write(ssl, &i, sizeof(int));
}

int comm_rdint ()
{
	int ret;
	SSL_read(ssl, &ret, sizeof(int));
	return ret;
}

int comm_wrfloat (float f)
{
	return SSL_write(ssl, &f, sizeof(float));
}

float comm_rdfloat ()
{
	float ret;
	SSL_read(ssl, &ret, sizeof(float));
	return ret;
}

int comm_srv_init (int argc, char **argv)
{
	if (argc<4)
		return -1;
	
	setvbuf(stdout, NULL, _IONBF, 0);
	sock_open(&cl, argv[1], atoi(argv[2]));
	if (sock_con(&cl))
		return -1;
	ssl_srv_init();
	if (strlen(argv[3])<(SEC_LEN-1)*2)
		return -1;
	SSL_write(ssl, argv[3], SEC_LEN);
	return 0;
}

int comm_srv_deinit ()
{
	usleep(10);
	comm_wrch(MANGO);
	sock_close(&cl, false);
	return ssl_srv_deinit();
}

static int passwd_cb (char *buf, int size, int rwflag, void *userdata)
{
	strcpy(buf, PASSWD);
	return strlen(PASSWD);
}
	
static int ssl_init ()
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

static int ssl_deinit ()
{
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}

static int ssl_srv_init ()
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

static int ssl_srv_deinit ()
{
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}

static char *gen_secret ()
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

static bool check_secret ()
{
	char *sec = (char *)malloc(SEC_LEN+1);
	if (SSL_read(ssl, sec, SEC_LEN)<SEC_LEN)
		return false;
	if (!memcmp(secret, sec, SEC_LEN))
		return true;
	return false;
}
