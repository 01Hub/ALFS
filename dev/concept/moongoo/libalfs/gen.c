#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include <libtar.h>

#include <gen.h>

const char *compr[NUM_COMPR] = { ".bz2", ".gz" };
const char *unpck[NUM_COMPR] = { "tar xfj ", "tar xfz " };

char *directory (char *tarball)
{
	char *ret;
	TAR *t = NULL;

	tar_open(&t, tarball, NULL, O_RDONLY, 0, 0);
	
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
