#ifndef __SSL_H__
#define __SSL_H__

#include <openssl/ssl.h>

extern SSL *ssl;

int ssl_init();
int ssl_deinit();
int ssl_srv_init();
int ssl_srv_deinit();

#endif
