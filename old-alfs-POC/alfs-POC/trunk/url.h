#ifndef	__URL_H__
#define __URL_H__

#include <libalfs.h>

/** Print all package URLs for a profile
@param fname Filename of a package info XML document
@param prof Profile
*/
void find_urls (char *fname, profile *prof);
/** Print all URLs contained in a Docbook XML subtree
@param bnode Root XML node
*/
void book_urls (xmlNodePtr bnode);

#endif
