#ifndef __NET_H__
#define __NET_H__

#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/types.h>

#define HTTP_OK			200

typedef enum
{
	PROTO_NONE = 1,
	HTTP,
	FTP
} protocol;

typedef struct
{
	char *mem;
	size_t size;
	CURLcode res;
} memchunk;

typedef struct
{
	char *proto;
	char *host;
	char *fname;
} URL;

memchunk *http_get (char *url);
memchunk *http_header (char *url);
memchunk *ftp_get (char *url);
CURLcode ftp_list (char *url);
protocol check_proto (char *url);
URL *parse_url (char *url);

#endif
