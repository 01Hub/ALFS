#ifndef __BUILD_H__
#define __BUILD_H__

#include <alfs.h>

/** Arguments for a build command sed */
typedef struct 
{
	char *name;	/** Current package name */
	char **filter;	/** Commands which should not be touched */
	char **p1;	/** Packages which need special treatment */
	char **p2;	/** Replacement strings for the relevant packages */
} sed_arg;

extern replaceable *r;

/** Build a package
@param Package information structure
@return 0 on success, -1 if an error occured.
*/
int build_pkg (package pkg);
/** Insert commands for paralell building all packages in a profile
@param prof Profile
@param filter Command filter
@param p1 Packages which need special treatment
@param p2 Replacement strings for the relevant packages
*/
void sed_paralell (profile *prof, char **filter, char **p1, char **p2);

#endif
