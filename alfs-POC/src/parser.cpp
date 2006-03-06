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

  int loc, len, i, multi = 0, comment = 0;
  string fn, parent, curline, path, buf, multibuf, parsebuf;
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

    if (comment == 1) {
      if ((curline.find("-->", 0)) == string::npos)
        continue;
      comment = 0;
      continue;
    }

    for (buf = string(curline); !buf.empty(); i = 0) {
	if (multi != 1)
	  i = buf.find_first_of("<");
	else
	  i = 0;

	switch (i) {

	  case -1 :
	  // '<' not found in the string
	    cout << buf << endl;
	    buf.erase();
	    break;

	  case 0 :
	    if ((buf.find("<!--", 0)) == 0) {
	      if ((buf.find("-->", 0)) == string::npos) {
		comment = 1;
	      }
	      buf.erase();
	    }
	  // '<' found as the first character of the string
	    else {
	      if (multi == 1) {
		multibuf.append(buf);
		buf = string(multibuf);
	      }
	      i = buf.find_first_of(">");
	      if (i != -1) {
	        // Analyze tag
	        parsebuf = string(buf, 0, i+1);
	        parsebuf = string(parsebuf, parsebuf.find_first_not_of("<"), (parsebuf.find_last_not_of(">")-parsebuf.find_first_not_of("<"))+1);
	        cout << "Parsed tag is: " << parsebuf << endl;
	        // cout << (string(buf, 0, i+1)) << endl;
	        buf = string(buf, i+1, buf.length()-i);
	        multi = 0;
	      } else {
	        // FIXME: append to previous line.
	        multibuf = string(buf);
	        multi = 1;
	        buf.erase();
	      }
	    }
	    break;

	  default :
	  // '<' found, but not the first character in the string.
	    if ((string(buf,0,i).find_first_not_of(" ")) != string::npos)
	      cout << (string(buf, 0, i)) << endl;
	    buf = string(buf, i, buf.length()-i);

	    
	}
	
    }
   // cout << "Current line: " << curline << endl;
  }

  fp.close();

  return(0);
}
