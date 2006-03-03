#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int parse_file(string filename);

int main (int argc, char **argv)
{
  /* Check that we have an input file */
  if (argc != 2){
        printf("Usage: %s [FILE]\n", argv[0]);
        return(-1);
  }

  /* We've been given a file, let's start parsing */
  string fn = argv[1];
  if ((parse_file(fn)) == -1)
        return(-1);

  return(0);

}

int parse_file(string filename){

  int loc, len, i, multi = 0;
  string fn, parent, curline, path, buf;
  const char *dir, *file;
  char wd[256];
  ifstream fp;
  
  len = filename.length();

  /* Isolate the path to the file from the actual filename. */
  if ((loc = filename.find_last_of("/")) == -1)
    fn = filename;  
  else {
    path = string(filename, 0, loc);
    fn = string(filename, loc+1, len-loc);
    dir = path.c_str();
    if ((chdir(dir)) == -1)
	cerr << "Cannot change directories to " << path << endl;
  }
 
  /* Open the file for parsing. */ 
  cout << "Parsing file: " << fn << endl;
  file = fn.c_str();
  fp.open(file);
  if (!fp) {
    perror(file);
    return(-1);
  }
  /* Set the current working directory, in case we leave it,
     and so we can get the name of the parent directory for
     the file we're parsing */
  getcwd(wd, 256);
  path = string(wd);
  if ((loc = path.find_last_of("/")) != -1)
      parent = string(path, 0, loc);

  /* Read one line at a time from the opened file
     until the end of the file is reached */
  while (fp.good()) {
    getline(fp, curline);
    if (curline.empty())
      continue;

    for (buf = string(curline); buf.empty() != 1; i = 0) {
	if (multi != 1)
	  i = buf.find_first_of("<");
	else
	  i = 0;

	switch (i) {
	  case -1 :
	    cout << buf << endl;
	    buf.erase();
	    break;

	  case 0 :
	    i = buf.find_first_of(">");
	    if (i != -1) {
	      cout << (string(buf, 0, i+1)) << endl;
	      buf = string(buf, i+1, buf.length()-i);
	      multi = 0;
	    } else {
	      cout << buf << endl;
	      multi = 1;
	      buf.erase();
	    }
	    break;

	  default :
	    cout << (string(buf, 0, i)) << endl;
	    buf = string(buf, i, buf.length()-i);
	    
	}
	
    }
   // cout << "Current line: " << curline << endl;
  }

  fp.close();

  return(0);
}
