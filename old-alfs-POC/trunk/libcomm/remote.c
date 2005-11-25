#include <libcomm.h>
#include <remote.h>

/* Functions for reading/writing the basic 'alfs.h' data-types. */

static void dependency_writer (gpointer data, gpointer user_data)
{
	dep *d = (dep *)data;
	comm_wrint(d->type);
	comm_wrstr(d->name);
}

static void download_writer (gpointer data, gpointer user_data)
{
	download *d = (download *)data;
	comm_wrint(d->algo);
	comm_wrint(d->proto);
	comm_wrstr(d->url);
	comm_wrstr(d->sum);
}

static void command_writer (gpointer data, gpointer user_data)
{
	command *c = (command *)data;
	comm_wrstr(c->cmd);
	comm_wrlist(c->arg, (GFunc)def_str_writer, NULL);
	comm_wrint(c->role);
}

static void package_writer (gpointer data, gpointer user_data)
{
	package *p = (package *)data;
	comm_wrstr(p->name);
	comm_wrstr(p->vers ? p->vers : "");
	comm_wrlist(p->build, command_writer, NULL);
	comm_wrlist(p->dl, download_writer, NULL);
	comm_wrlist(p->dep, dependency_writer, NULL);
}

static void chapter_writer (gpointer data, gpointer user_data)
{
	chapter *c = (chapter *)data;
	comm_wrstr(c->name);
	comm_wrstr(c->ref);
	comm_wrlist(c->pkg, package_writer, NULL);
}

void profile_writer (profile *p)
{
	comm_wrstr(p->name);
	comm_wrstr(p->vers);
	comm_wrlist(p->ch, chapter_writer, NULL);
}

static dep *dependency_reader ()
{
	dep *d = (dep *)malloc(sizeof(dep));
	d->type = comm_rdint();
	d->name = comm_rdstr();
	return d;
}

static download *download_reader ()
{
	download *d = (download *)malloc(sizeof(download));
	d->algo = comm_rdint();
	d->proto = comm_rdint();
	d->url = comm_rdstr();
	d->sum = comm_rdstr();
	return d;
}

static command *command_reader ()
{
	command *c = (command *)malloc(sizeof(command));
	c->cmd = comm_rdstr();
	c->arg = comm_rdlist((reader_t)comm_rdstr);
	c->role = comm_rdint();
	return c;
}

static package *package_reader ()
{
	package *p = (package *)malloc(sizeof(package));
	p->name = comm_rdstr();
	p->vers = comm_rdstr();
	p->build = comm_rdlist((reader_t)command_reader);
	p->dl = comm_rdlist((reader_t)download_reader);
	p->dep = comm_rdlist((reader_t)dependency_reader);
	return p;	
}

static chapter *chapter_reader ()
{
	chapter *c = (chapter *)malloc(sizeof(package));
	c->name = comm_rdstr();
	c->ref = comm_rdstr();
	c->pkg = comm_rdlist((reader_t)package_reader);
	return c;
}

profile *profile_reader ()
{
	profile *p = (profile *)malloc(sizeof(profile));
	p->name = comm_rdstr();
	p->vers = comm_rdstr();
	p->ch = comm_rdlist((reader_t)chapter_reader);
	return p;	
}
