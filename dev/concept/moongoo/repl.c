#include <string.h>

#include <repl.h>
#include <alfs.h>

replaceable r[REPLC];

void t_repl (xmlNodePtr node, void *data)
{
	int i;
	xmlNodePtr text = find_node(node->children, "text");
	char *repl = xmlNodeGetContent(text);
	
	for (i=0;i<REPLC;i++)
		if (!strcmp(repl, r[i].orig))
		{
			xmlNodeSetContent(node, r[i].repl);
			return;
		}
	
	fprintf(stderr, "Unknown replaceable: %s\n", repl);
}

// TODO: Make <replaceable> actually configurable
void init_repl ()
{
	// LFS install partition
	r[0].orig="[xxx]";
	r[0].repl="/dev/discs/disc0/part5";
	r[14].orig="[fff]";
	r[14].repl="ext3";
	// LFS user partition
	r[12].orig="[zzz]";
	r[12].repl="";
	// Swap partition
	r[1].orig="[yyy]";
	r[1].repl="/dev/discs/disc0/part10";
	// Paper size
	r[2].orig="[paper_size]";
	r[2].repl="A4";
	// Network settings
	r[3].orig="[FQDN]";
	r[3].repl="Lenin.Ass";
	r[4].orig="[HOSTNAME]";
	r[4].repl="Lenin";
	r[5].orig="[192.168.1.1]";
	r[5].repl="192.168.1.23";
	r[10].orig="[Your Domain Name]";
	r[10].repl="Ass";
	r[11].orig="[IP address of your nameserver]";
	r[11].repl="217.237.149.161";
	// Locales
	r[6].orig="[ll]";
	r[6].repl="de";
	r[7].orig="[CC]";
	r[7].repl="DE@euro";
	r[8].orig="[arguments for loadkeys]";
	r[8].repl="de-latin1-nodeadkeys";
	r[9].orig="[arguments for setfont]";
	r[9].repl="lat9u-16";
	r[13].orig="[/path/to/your/keymap]";
	r[13].repl="";
	// Pathes
	r[15].orig="[unpacked sources dir]";
	r[15].repl="/usr/src";
}	
