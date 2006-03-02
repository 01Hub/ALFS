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

  int loc, len;
  string fn, parent, curline;
  const char *dir, *file;
  char wd[512];
  ifstream fp;
  
  len = filename.length();

  /* Isolate the path to the file from the actual filename. */
  if ((loc = filename.find_last_of("/")) == -1)
    fn = filename;  
  else {
    string path(filename, 0, loc);
    fn = string(filename, loc+1, len-loc);
    if ((loc = path.find_last_of("/")) != -1)
       parent = string(path, 0, loc);
    dir = path.c_str();
    if ((chdir(dir)) == -1)
	cerr << "Cannot change directories to " << path << endl;
  }
 
  /* Open the file for parsing. */ 
  cout << "Parsing file: " << fn << endl;
  file = fn.c_str();
  fp.open(file);
  if (!fp)
    cerr << "Cannot open file: " << fn << endl;

  /* Set the current working directory, in case we leave it,
     and so we can get the name of the parent directory for
     the file we're parsing */
  getcwd(wd, 256);

  /* Read one line at a time from the opened file
     until the end of the file is reached */
  while (!fp.eof()) {
    getline(fp, curline);
    cout << "Current line: " << curline << endl;	
  }

  fp.close();

  return(0);
}
