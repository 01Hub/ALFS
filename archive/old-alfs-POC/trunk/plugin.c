#include <dlfcn.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>

#include <libalfs.h>
#include <plugin.h>

/* The plugin code is based on hyperplayer by James Lee (jbit) */

static void plugerr (char *fname);

plug_info *plugscan (char *dir)
{
	DIR *d;
	struct dirent *ent;
	plug_info *ret = NULL;
	int num = 0;

	d = opendir(dir);
	if (!d)
	{
		perror(dir);
		return NULL;
	}

	ent = readdir(d);
	while (ent)
	{
		if (!strcmp(&ent->d_name[strlen(ent->d_name)-strlen(PLUG_EXT)],
			PLUG_EXT))
		{
			char *path = strdog_path(dir, ent->d_name);
			plug_info *moo = pluginfo(path);
			
			if (moo)
			{
				ret = realloc(ret, (++num)*sizeof(plug_info));
				ret[num-1] = *pluginfo(path);
				ret[num-1].path = path;
			}
			else
				free(path);
		}

		ent = readdir(d);
	}

	ret = realloc(ret, (++num)*sizeof(plug_info));
	ret[num-1].info = NULL;
	ret[num-1].path = NULL;

	closedir(d);
	return ret;
}

plug_info *pluginfo (char *fname)
{
	plug_info *ret = (plug_info *)malloc(sizeof(plug_info));
	plug_hand *hand = plugload(fname);
	
	if (!hand)
		return NULL;
	
	ret->info = hand->getplug();
	if (!ret->info)
	{
		fprintf(stderr, "No plugin info in %s.\n", fname);
		dlclose(hand->hand);
		return NULL;
	}

	if (ret->info->vers!=PLUG_VER)
	{
		fprintf(stderr, "Incompatible plugin version %d.\n", 
			ret->info->vers);
		dlclose(hand->hand);
		return NULL;
	}

	ret->hand = hand->hand;
	return ret;
}

plug_hand *plugload (char *fname)
{
	plug_hand *ret = (plug_hand *)malloc(sizeof(plug_hand));
	
	ret->hand = dlopen(fname, RTLD_NOW);
	if (!ret->hand)
	{
		plugerr(fname);
		return NULL;
	}

	ret->getplug = dlsym(ret->hand, "getplug");
	if (!ret->getplug)
	{
		plugerr(fname);
		return NULL;
	}

	return ret;
}

static void plugerr (char *fname)
{
	char *msg = dlerror();
	
	if (msg)
	{
		msg = strstr(msg, ":");
		msg+=2;
		fprintf(stderr, "%s: %s\n", fname, msg);
	}
}

void print_plug (plug_info plug)
{
	printf("Plugin: %s - Version: %d\n", plug.info->name, plug.info->vers);
	printf("Location: %s (%s)\n", plug.path, plugarg(plug.path));
}

char *plugarg (char *path)
{
	return strkill(basename(path), "." PLUG_EXT);
}

void plugunload (plug_info *plug)
{
	int i = 0;

	while (plug[i].path)
	{
		dlclose(plug[i].hand);
		i++;
	}
}

void print_plugs (plug_info *plugs, char *def)
{
	int i=0;
	
	while (plugs[i].info)
	{
		char *tmp = plugarg(plugs[i].path);
		if (strcmp(tmp, "sample"))
			printf("\t%s\t\t%s%s\n", tmp, plugs[i].info->name,
				(((def) && !strcmp(tmp, def)) ? " (default)" : ""));
		i++;
	}
}
