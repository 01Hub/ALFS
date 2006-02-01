/*
   Parser for the LFS Book. Grabs only executable commands.
   Writtem by Jeremy Huntwork, 20051229
*/
   
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 4096
#define CMDSDIR "commands"


int parse_file(char *filename);

int close_tag(char *line);

int close_comment(char *line);

int output_commands(char *dirname, char *filename);

char commands[1024];
int multi, comment, count=0;

int main (int argc, char **argv)
{
  /* Check that we have an input file */
  if (argc != 2){
	printf("Usage: %s [FILE]\n", argv[0]);
	return(-1);
  }

  /* We've been given a file, let's start parsing */
  if ((parse_file(argv[1])) == -1)
	return(-1);

  return(0);

}

int parse_file(char *filename) {

  FILE *fp;
  char buf[MAXLINE+1];
  char *href, *url, *loc, *loc2, *parent;
  char wd[512], path[512];
  int len;

  /* Get the path to the parent directory of filename;
     start by getting the last instance of a '/' */
  if ((loc = strrchr(filename, '/')) != NULL) {

  	/* Get the number position of the last '/' */
	len = loc-filename;

	/* Copy the path up to the last '/' to a new buffer */
	strncpy(path, filename, len);

	/* Add a terminating character to the end of the string */
	path[len] = '\0';
	
	/* Shorten the filename to be just the name of the file,
	   not the full path */
	filename = loc+1;

	/* Change directories to the path */
	if ((chdir(path)) == -1)
		perror(path);

  }

  printf("Parsing file: `%s\'\n", filename);

  /* Open the file for reading */
  fp = fopen(filename, "r");

  /* Make sure we have a readable file */
  if (fp == NULL){
	perror(filename);
	return(-1);
  }

  /* Add this file to the tally. */
  count++;
  printf("Files parsed: %d\n", count);

  /* Set the current working directory, in case we leave it,
     and so we can get the name of the parent directory for
     the file we're parsing */
  getcwd(wd, 256);

  /* Get the name of the parent directory only */
  if ((loc2 = strrchr(wd, '/')) == NULL) {
	perror(loc2);
	return(-1);
  }
  parent = loc2+1;
  
  /* Read one line at a time from the opened file
     until the end of the file is reached */
  while (!feof(fp)) {

	/* Put the current line into a buffer */
	fgets(buf, MAXLINE, fp);

	/* First let's check if we're continuing from a previous line.
	   That is, we either found an opening <screen> AND <userinput> and we
	   want to make sure we get all the stuff up to the closing tags,
	   or we found a comment. */

	if (comment == 1) {
		close_comment(buf);
		continue;
	}

	if (multi == 1) {

		/* Append the line to the current commands string */
		strcat(commands, buf);

		/* And see if we now have the closing tag */
		if ((close_tag(buf)) == -1)
			continue;
		output_commands(parent, filename);
		continue;
	}

	/* Seems we're not continuing from a previous line;
	   so check to see if this line is a comment. */
	if ((strstr(buf, "<!--")) != NULL)
	{
		close_comment(buf);
		continue;
	}
	/* Now let's see if we have <screen> AND <userinput> tags
	   which is the marker of a command to be run */
	if ((strstr(buf, "<screen>")) != NULL)
	{
	  /* Now let's make sure it's not marked as a role="nodump" */
	  if ((strstr(buf, "nodump")) != NULL)
		continue;

	  if ((strstr(buf, "<userinput>")) != NULL) {
		/* OK, we've got a command we want; Copy it to the commands
		   buffer. */
		strcpy(commands, buf);

		/* Now check to see if the closing tag is on the same line */
		if ((close_tag(buf)) == -1)
			continue;
		output_commands(parent, filename);
		continue;
	  }
	}

	/* No userinput tag, how about a 'xi:include'?
	   If there is a match, look for href */
	if ((strstr(buf, "xi:include")) != NULL) {

		/* Make sure it's not just grabbing a link to some text
		   in a future chapter.
		   FIXME: Find a better way, in case it's ever decided
		   later to use this tag to also pull in commands. */
		if ((strstr(buf, "xpointer")) != NULL)
			continue;

		/* If there is an href, we'll follow it,
		   but first we have to get the URL :-) */
		href = strstr(buf, "href");
		if (href != NULL) {			

			/* Initiate the strtok function - find
			   the first occurence of the delimiter,
			   i.e., the first quote */
			url = strtok(href,"\"");

		  	/* Now grab the first quote-delimited string,
			   which should invariably be the ... in href="..." */
			url = strtok(NULL, "\"");

			/* We have the URL to the file, so let's parse it */
			if ((parse_file(url)) == -1) {
				perror(url);
				return(-1);
			}

			/* We're back from parsing the included file; we need
			   to change back to our old dir before moving on to
			   the next line of the original file */
			if ((chdir(wd)) == -1)
				perror(wd);
		}		
	}
  }

  fclose(fp);
  
  return(0);
}

int close_tag(char *line) {
/* Find a closing </userinput> tag on the provided string
   and set the multi int accordingly */

  if ((strstr(line, "</userinput>")) == NULL) {
	multi=1;
	return(-1);
  }
  multi=0;
  return(0);
}

int close_comment(char *line) {
  if ((strstr(line, "-->")) == NULL) {
	comment=1;
	return(-1);
  }
  comment=0;
  return(0);
}

int output_commands(char *dirname, char *filename) {

  char *loc;
  char fullpath[512], strippedfile[256], num[4];
  int len;
  
  if (count < 100)
	sprintf(num, "0%d", count);
  else
        sprintf(num, "%d", count); 

  /* Strip the filename of '.xml' */
  loc = strchr(filename, '.');
  len = loc-filename;
  strncpy(strippedfile, filename, len);
  strippedfile[len] = '\0';

  /* Append each part of the path to a long string */
  strcpy(fullpath, CMDSDIR);
  strcat(fullpath, "/");
  strcat(fullpath, dirname);
  strcat(fullpath, "/");
  strcat(fullpath, num);
  strcat(fullpath, "-");
  strcat(fullpath, strippedfile);

  printf("PATH OF FILE TO WRITE: %s\n", fullpath);

  /* mkdir(CMDSDIR, 0755); */

  printf("---COMMANDS---\n%s\n", commands);

  return(0);

}
