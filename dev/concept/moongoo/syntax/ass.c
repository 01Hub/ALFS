#include <libgen.h>
#include <string.h>

#include <libalfs.h>
#include <plugin.h>

profile *prof;

profile *ass_profile (xmlNodePtr node, replaceable *r);

static t_plug ass_plugin =
{
	name:	"ALFS simple syntax",
	vers:	PLUG_VER,
	parse:	ass_profile
};

t_plug *getplug ()
{
	return &ass_plugin;
}


static void process_cmd (char *line)
{
	int i, j, k;

	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].build = realloc(prof->ch[i].pkg[j].build,
			(++prof->ch[i].pkg[j].n)*sizeof(command));
	k = prof->ch[i].pkg[j].n-1;
	
	prof->ch[i].pkg[j].build[k].role = ROLE_NONE;

	if (strcnt(line, " "))
	{
		prof->ch[i].pkg[j].build[k].cmd = strcut(line, 0, whereis(line, ' '));
		prof->ch[i].pkg[j].build[k].arg = tokenize(notrail(strstr(line, " "), 
				" "), " ", &prof->ch[i].pkg[j].build[k].n);
	}
	else
	{
		prof->ch[i].pkg[j].build[k].cmd = line;
		prof->ch[i].pkg[j].build[k].arg = NULL;
		prof->ch[i].pkg[j].build[k].n = 0;
	}
}

static  void t_shell (xmlNodePtr node, void *data)
{
	char *line = squeeze(xmlNodeGetContent(node));
	line = strkill(line, "\\\n");

	if (strcnt(line, "\n"))
	{
		char *tmp;

		while ((line) && (strlen(line)))
		{
			tmp = strsep(&line, "\n");
			process_cmd(tmp);
		}
	}
	else
		process_cmd(line);
}

static  void t_item (xmlNodePtr node, void *data)
{
	int i, j, k;
	download *moo;

	i = prof->n-1;
	j = prof->ch[i].n-1;
	prof->ch[i].pkg[j].dl = realloc(prof->ch[i].pkg[j].dl,
		(++prof->ch[i].pkg[j].m)*sizeof(download));
	k = prof->ch[i].pkg[j].m-1;
	moo = &prof->ch[i].pkg[j].dl[k];

	if (xmlGetProp(node, "sha1"))
	{
		moo->algo = SHA1;
		moo->sum = xmlGetProp(node, "sha1");
	}
	else
	if (xmlGetProp(node, "md5"))
	{
		moo->algo = MD5;
		moo->sum = xmlGetProp(node, "md5");
	}
	else
	{
		moo->algo = ALGO_NONE;
		moo->sum = NULL;
	}
	
	moo->proto = parse_proto(node);
	moo->url = xmlNodeGetContent(node);
}

static  void t_down (xmlNodePtr node, void *data)
{
	foreach(node->children, "item", (xml_handler_t)t_item, NULL);
}

static  void t_unpack (char *url)
{
	char *ext = extonly(url), *ret = NULL;

	if (!strcmp(ext, ".tar.gz"))
		ret = "tar xfz ";
	if (!strcmp(ext, ".tar.bz2"))
		ret = "tar xfj ";

	if (!ret)
	{
		fprintf(stderr, "Archive extension '%s' is unknown.\n", ext);
		return;
	}

	ret = strdog(ret, basename(url));
	process_cmd(ret);
	free(ret);
}

static  void t_page (xmlNodePtr node, void *data)
{
	int i, j;
	xmlNodePtr dir = find_node(node->children, "directory");

	i = prof->n-1;
	prof->ch[i].pkg = realloc(prof->ch[i].pkg,
			(++prof->ch[i].n)*sizeof(package));
	j = prof->ch[i].n-1;

	prof->ch[i].pkg[j].name = find_value(node, "title");
	prof->ch[i].pkg[j].vers = find_value(node, "version");
	prof->ch[i].pkg[j].build = NULL;
	prof->ch[i].pkg[j].n = 0;
	prof->ch[i].pkg[j].dl = NULL;
	prof->ch[i].pkg[j].m = 0;

	foreach(node->children, "download", (xml_handler_t)t_down, NULL);
	t_unpack(prof->ch[i].pkg[j].dl[0].url);
	process_cmd(strdog("__cd ", xmlNodeGetContent(dir)));
	foreach(node->children, "shell", (xml_handler_t)t_shell, NULL);
}

static  void t_chapter (xmlNodePtr node, void *data)
{
	prof->ch = realloc(prof->ch, (++prof->n)*sizeof(chapter));
	prof->ch[prof->n-1].name = xmlGetProp(node, "name");
	prof->ch[prof->n-1].ref = xmlGetProp(node, "ref");
	prof->ch[prof->n-1].pkg = NULL;
	prof->ch[prof->n-1].n = 0;
	foreach(node->children, "page", (xml_handler_t)t_page, NULL);
}

profile *ass_profile (xmlNodePtr node, replaceable *r)
{
	node = find_node(node, "ass");

	if (!node)
	{
		fprintf(stderr, "XML document is not a valid ASS profile.\n");
		return NULL;
	}
	
	prof = (profile *)malloc(sizeof(profile));
	prof->name = find_value(node, "title");
	prof->vers = find_value(node, "version");
	prof->ch = NULL;
	prof->n = 0;
	foreach(node->children, "chapter", (xml_handler_t)t_chapter, NULL);
	return prof;
}
