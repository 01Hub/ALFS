#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int parse_file(string filename);

int add_entity(string ent_pair);

string swap_ent(string ent_value);
string find_ent(string ent_name);
string parsed_ent_files;

struct entities {
  string name;
  string value;
  entities* next;
}; 

entities *head = NULL;

int main (int argc, char **argv)
{
  /* Check that we have an input file */
  if (argc != 2){
        printf("Usage: %s [FILE]\n", argv[0]);
        return(-1);
  }

  /* We've been given a valid file */
  /* First add a few static entities */
  add_entity("gt \">\"");
  add_entity("lt \"<\"");
  add_entity("amp \"&\"");
  add_entity("quot \"\"\"");

  /* Now parse the file */
  string fn = argv[1];
  if ((parse_file(fn)) == -1)
        return(-1);

  return(0);

}

int parse_file(string filename){

  int loc, len, i, multi = 0, comment = 0, screen = 0, userinput = 0;
  string fn, parent, curline, path, buf, multibuf, parsebuf, url;
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
  // cout << "Parsing file: " << fn << endl;
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

    /* Look for XML tags */
    for (buf = string(curline); !buf.empty(); i = 0) {

	if (multi != 1) // We're not in the middle of a multi-line tag
	  i = buf.find_first_of("<");
	else
	  i = 0;

	switch (i) {

	  case -1 :
	  // '<' not found in the string
	    if ((screen == 1) && (userinput == 1)) {
		buf = swap_ent(buf);
	  	cout << buf << endl;
	    }
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
		loc = parsebuf.find_first_not_of("<");
		len = parsebuf.find_last_not_of(">");
	        parsebuf = string(parsebuf, loc, (len-loc)+1);
	        //cout << "Parsed tag is: " << parsebuf << endl;
	
		// Do we have an entity?
		if ((parsebuf.find("!ENTITY")) != string::npos) {
		  if ((parsebuf.find("%")) != string::npos) {
		    //process the system entity
		    url = string(parsebuf, parsebuf.find("SYSTEM"), parsebuf.length()-parsebuf.find("SYSTEM")+1);
		    url = string(url, url.find_first_of("\"")+1, url.find_last_of("\"")-url.find_first_of("\"")-1);
		    if ((parsed_ent_files.find(url)) == string::npos) {
		      //parse the included system entity
		      if ((parse_file(url.c_str())) == -1) {
			perror(url.c_str());
			return(-1);
	              } else {
			parsed_ent_files.append(url);
		      }
		      if ((chdir(wd)) == -1)
			perror(wd);
		    }
		  } else {
		    parsebuf.erase(0, 8);
		    add_entity(parsebuf);
		  }
		}

		// Do we have an xi:include?
		if ((parsebuf.find("xi:include")) != string::npos) {
		  //skip any xpointer stuff for now - will get to it in a bit
		  if ((parsebuf.find("xpointer")) == string::npos) {
		    url = string(parsebuf, parsebuf.find("href="), parsebuf.length()-parsebuf.find("href=")+1);
		    url = string(url, url.find_first_of("\"")+1, url.find_last_of("\"")-url.find_first_of("\"")-1);
		    //cout << "Got an included file! URL is: " << url << endl;
		    //parse the file
		    if ((parse_file(url.c_str())) == -1) {
                      perror(url.c_str());
                      return(-1);
                    }
                    if ((chdir(wd)) == -1)
                        perror(wd);
		  } 
		}

		// Do we have a screen tag without the nodump role?
		if ((parsebuf.find("screen")) != string::npos) {
		   if ((parsebuf.find("nodump")) == string::npos){
			screen = 1;
		   }
		}

		if ((parsebuf.find("userinput")) != string::npos)
			userinput = 1;

		if ((parsebuf.find("/userinput")) != string::npos) {
			userinput = 0;
			cout << endl;
		}
		if ((parsebuf.find("/screen")) != string::npos)
			screen = 0;

	        buf = string(buf, i+1, buf.length()-i);
	        multi = 0;
	      } else {
	        multibuf = string(buf);
	        multi = 1;
	        buf.erase();
	      }
	    }
	    break;

	  default :
	  // '<' found, but not the first character in the string.
	  //  if ((string(buf,0,i).find_first_not_of(" ")) != string::npos)
	  //    cout << (string(buf, 0, i)) << endl;
            if ((screen == 1) && (userinput == 1)) {
                parsebuf = string(buf, 0, i);
                parsebuf = swap_ent(parsebuf);
                cout << parsebuf;
            }
	    buf = string(buf, i, buf.length()-i);

	    
	}
	
    }
   // cout << "Current line: " << curline << endl;
  }

  fp.close();

  return(0);
}

int add_entity(string ent_pair) {
  entities *list, *temp;
  string ent_name, ent_value;
  int loc, loc1;

  // Isolate the entity name - at this point, it's everything up to the
  // first space.
  ent_name = string(ent_pair, 0,  ent_pair.find_first_of(" "));

  // Isolate the entity value - everything in quotes
  loc = ent_pair.find_first_of("\"");
  loc1 = ent_pair.find_last_of("\"");
  ent_value = string(ent_pair, loc+1, loc1-loc-1);

  ent_value = swap_ent(ent_value);

  // Our entity is ready to be added to the linked list
  temp = new entities;
  temp->name = string(ent_name);
  temp->value = string(ent_value);
  temp->next = NULL;

  if (head == NULL)
    head = temp;
  else {
    list = head;
    while (list->next != NULL)
      list = list->next;
    list->next = temp;
  }
  // cout << "Added entity: " << ent_name << "=\"" << ent_value << "\"" << endl;

  return(0);
}

// Function to search a linked list of entities and return the
// value of the found matching entity name
string find_ent(string ent_name){
 string ent_value = "none";
 entities *temp;
 temp = head;
 while (temp != NULL) {
  if (temp->name.compare(ent_name) == 0) {
    ent_value = string(temp->value);
    break;
  }
  temp = temp->next;
 }
 return(ent_value);
}

// Function to swap the entity name with the entity value
string swap_ent(string line){
  string start_ent, get_ent, buf;
  int loc, loc1, ent_len;

  // For each instance of '&...;' in value search the linked list of
  // entities for a matching entity - if nothing is found leave the original
  // value intact.
  while ( ! line.empty()) {
    if ((loc = line.find_first_of("&")) == -1 )
	break;
    start_ent = string(line, loc+1, line.length());
 
    if ((loc1 = start_ent.find_first_of(";")) == -1)
	break;
    
    get_ent = string(start_ent, 0, loc1);
    ent_len = get_ent.length();
    get_ent = find_ent(get_ent);

    if (get_ent.compare("none") != 0) {
      buf.append(string(line, 0, loc));
      buf.append(get_ent);
      line = string(line, loc+ent_len+2, line.length());
    } else {
       break;
    }
  }
  buf.append(line);
  line = string(buf);
  return(line);
}
