/*
 *  digest.c - Functions for handling file digests.
 * 
 *  Copyright (C) 2001-2003
 *  
 *  Neven Has <haski@sezampro.yu> and
 *  Kevin P. Fleming <kpfleming@linuxfromscratch.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <nALFS.h>

#ifdef HAVE_LIBSSL

#include <openssl/evp.h>
#define MAX_MD_SIZE EVP_MAX_MD_SIZE

#else /* !HAVE_LIBSSL */

#define MAX_MD_SIZE 16

#endif /* HAVE_LIBSSL */

#include "win.h"
#include "md5.h"
#include "parser.h"
#include "utility.h"

int verify_digest(const char* type, const char* digest, const char* file)
{
	char buffer[4094];
	FILE *istream;
	unsigned int i;
	unsigned int md_len = 0;
	unsigned char md_value[MAX_MD_SIZE];
	char md_value_hex[2*MAX_MD_SIZE + 1 ];
	int use_ssl;

        struct md5_context md5_ctx;

#ifdef HAVE_LIBSSL
        EVP_MD_CTX ssl_ctx;
	const EVP_MD *ssl_md;
#endif /* HAVE_LIBSSL */

	use_ssl = strncmp(type, "md5", 3);

	if (!use_ssl) {
		md5_starts(&md5_ctx);
	} else {
#ifdef HAVE_LIBSSL
		OpenSSL_add_all_digests();
		ssl_md = EVP_get_digestbyname(type);
		if (ssl_md)
			EVP_DigestInit(&ssl_ctx, ssl_md);
		else {
			Nprint_err("SSL does not support %s digest", type);
			return -1;
		}
#else /* !HAVE_LIBSSL */
		Nprint_err("Only MD5 digest supported without SSL");
		return -1;
#endif /* HAVE_LIBSSL */
	}

	istream = fopen(file, "r");
	if (istream) {
		size_t nbbytes;

		while ((nbbytes = fread(&buffer[0], 1, sizeof(buffer), istream)) != 0) {
			if (!use_ssl)
				md5_update(&md5_ctx, &buffer[0], nbbytes);
#ifdef HAVE_LIBSSL
			else
			        EVP_DigestUpdate(&ssl_ctx, &buffer[0], nbbytes);
#endif /* HAVE_LIBSSL */
		}

		if (!use_ssl) {
			md5_finish(&md5_ctx, &md_value[0]);
			md_len = 16;
		}
#ifdef HAVE_LIBSSL
		else
			EVP_DigestFinal(&ssl_ctx, &md_value[0], &md_len);
#endif /* HAVE_LIBSSL */

		fclose(istream);

		for(i = 0; i < md_len; i++)
	                sprintf(&md_value_hex[0] + i * 2, "%02x", md_value[i]);

		if (! strcmp(digest, &md_value_hex[0])) {
			Nprint("Digest ok.");
		        return 0;
		}

		Nprint_err("Expected digest : %s", digest);
		Nprint_err("Found digest    : %s", &md_value_hex[0]);

	} else {
		Nprint_err(
		"Unable to open the archive when checking digest: %s", file);
	}

	return -1;
}
