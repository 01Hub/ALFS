#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bzlib.h>
#include <libtar.h>
#include <zlib.h>

#include <libalfs.h>

const char *compr[NUM_COMPR] = { ".bz2", ".gz" };
const char *unpck[NUM_COMPR] = { "tar xfj ", "tar xfz " };

/* This code is partially taken from libtar, (C) 1998-2003  Mark D.Roth */
int bzopen_fe (char *pathname, int oflags, int mode);
tartype_t bz2type = { (openfunc_t) bzopen_fe, (closefunc_t) BZ2_bzclose, 
	(readfunc_t) BZ2_bzread, (writefunc_t) BZ2_bzwrite };
int gzopen_fe (char *pathname, int oflags, int mode);
tartype_t gztype = { (openfunc_t) gzopen_fe, (closefunc_t) gzclose,
	(readfunc_t) gzread, (writefunc_t) gzwrite };

int bzopen_fe (char *pathname, int oflags, int mode)
{
	char *bzflags;
	BZFILE *bzf;
	int fd;

	switch (oflags & O_ACCMODE)
	{
		case O_WRONLY:
			bzflags="wb";
			break;
		case O_RDONLY:
			bzflags="rb";
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	fd = open(pathname, oflags, mode);
	if (fd==-1)
		return -1;

	if ((oflags & O_CREAT) && fchmod(fd, mode))
		return -1;

	bzf = BZ2_bzdopen(fd, bzflags);
	if (!bzf)
	{
		errno = ENOMEM;
		return -1;
	}

	return (int)bzf;
}

int gzopen_fe (char *pathname, int oflags, int mode)
{
	char *gzoflags;
	gzFile gzf;
	int fd;

	switch (oflags & O_ACCMODE)
	{
		case O_WRONLY:
			gzoflags = "wb";
			break;
		case O_RDONLY:
			gzoflags = "rb";
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	fd = open(pathname, oflags, mode);
	if (fd == -1)
		return -1;

	if ((oflags & O_CREAT) && fchmod(fd, mode))
		return -1;

	gzf = gzdopen(fd, gzoflags);
	if (!gzf)
	{
		errno = ENOMEM;
		return -1;
	}

	return (int)gzf;
}

char *directory (char *tarball)
{
	char *ret, *ext;
	TAR *t = NULL;
	tartype_t *moo = NULL;

	ext = strrchr(extonly(tarball), '.');
	if (ext)
	{
		ext++;
		if (!strcmp(ext, "gz"))
			moo = &gztype;
		if (!strcmp(ext, "bz2"))
			moo = &bz2type;
	}
	
	tar_open(&t, tarball, moo, O_RDONLY, 0, 0);
	
	if (!t)
		return NULL;
	
	th_read(t);
	ret = th_get_pathname(t);
	tar_close(t);

	return ret;
}

char *read_file (char *fname)
{
	FILE *f;
	int sz=0;
	char c, *ch=calloc(1, sizeof(char));

	if (!fname)
	{
		free(ch);
		return NULL;
	}

	f=fopen(fname, "r");
	if (!f)
	{
		perror(fname);
		free(ch);
		return NULL;
	}
	
	while ((c=fgetc(f))!=EOF)
	{
		ch=realloc(ch, (++sz)*sizeof(char));
		ch[sz-1]=c;
	}

	ch=realloc(ch, (++sz)*sizeof(char));
	ch[sz-1]='\0';

	return ch;
}

char *popen_read (char *cmd)
{
	char *ch=NULL, c;
	int sz=0;
	FILE *moo;

	if (!cmd)
		return NULL;

	moo = popen(cmd, "r");
	if (!moo)
	{
		perror("popen_read()");
		return NULL;
	}
		
	while ((c=fgetc(moo))!=EOF)
	{
		ch=realloc(ch, (++sz)*sizeof(char));
		ch[sz-1]=c;
	}

	ch=realloc(ch, (++sz)*sizeof(char));
	ch[sz-1]='\0';

	pclose(moo);
	return ch;
}
