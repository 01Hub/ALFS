#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net.h>
#include <util.h>

/* This code is based on the libcurl examples */

size_t writemem_cb (void *ptr, size_t size, size_t nmemb, void *data)
{
	register int realsize = size * nmemb;
	memchunk *mem = (memchunk *)data;

	mem->mem = (char *)realloc(mem->mem, mem->size + realsize + 1);
	if (mem->mem)
	{
		memcpy(&(mem->mem[mem->size]), ptr, realsize);
		mem->size += realsize;
		mem->mem[mem->size] = 0;
	}

	return realsize;
}

static memchunk *__url_get (char *url, bool header, protocol proto)
{
	CURL *handle;
	memchunk *chunk = (memchunk *)malloc(sizeof(memchunk));

	chunk->mem = NULL;
	chunk->size = 0;

	curl_global_init(CURL_GLOBAL_ALL);
	handle = curl_easy_init();

	if (header)
	{
		if (proto==HTTP)
			curl_easy_setopt(handle, CURLOPT_HEADER, 1);
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	}
	
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, writemem_cb);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)chunk);
	curl_easy_setopt(handle, CURLOPT_USERAGENT, "moongoo-agent/1.0");
	//curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
	
	chunk->res = curl_easy_perform(handle);
	curl_easy_cleanup(handle);

	if ((header) && (proto==HTTP))
	{
		char *head = chrep(chunk->mem, '\n', '\0'), *tmp;
		tmp = strstr(head, " ");
		chunk->res = atoi(++tmp);
		free(head);
	}

	return chunk;
}

memchunk *http_get (char *url)
{
	// TODO: Make HTTP get usable
	return __url_get(url, false, HTTP);
}

memchunk *http_header (char *url)
{
	return __url_get(url, true, HTTP);
}

memchunk *ftp_get (char *url)
{
	// TODO: Implement FTP get
	return NULL;
}

URL *parse_url (char *url)
{
	URL *ret = (URL *)malloc(sizeof(URL));
	ret->proto = chrep(url, ':', '\0');
	ret->host = chrep(strnstr(url, "/", 2), '/', '\0');
	ret->fname = strnstr(url, "/", 3);
	return ret;
}

static CURLcode __ftp_list (char *url, char *command)
{
	CURL *handle;
	URL *moo = parse_url(url);
	CURLcode res;
	struct curl_slist *cmd = NULL;
	
	cmd = curl_slist_append(cmd, strdog(command, moo->fname));
	
	curl_global_init(CURL_GLOBAL_DEFAULT);
	handle = curl_easy_init();

	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_QUOTE, cmd);
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);
	//curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);

	res = curl_easy_perform(handle);
	if (res!=CURLE_OK)
		curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &res);

	curl_slist_free_all(cmd);
	curl_easy_cleanup(handle);
	curl_global_cleanup();
	return res;
}

CURLcode ftp_list (char *url)
{
	CURLcode ret = __ftp_list(url, "SIZE ");
	if ((ret>500) && (ret!=550))
		ret = __ftp_list(url, "STAT ");
	return ret;
}

protocol check_proto (char *url)
{
	if (!strncmp(url, "http", 4))
		return HTTP;
	else
	if (!strncmp(url, "ftp", 3))
		return FTP;
	
	return PROTO_NONE;
}
