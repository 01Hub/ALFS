#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>

char *lower_case (char *str);
char *strdog (char *str1, char *str2);
char *chrep (char *str, char old, char new);
char **tokenize (char *str, char *token, int *i);
char *strrep (char *str, char *old, char *new);
int strcnt (char *str, char *sstr);
int strstarts (char *str, int begin, char *test);
char *strcutb (char *str, int begin, int count, bool blanks);
char *strcut (char *str, int begin, int count);
int whereis (char *str, char ch);
char *notrail (char *str, char *token);
char *squeeze (char *str);
char *strkill (char *str, char *tokill);
char *strnstr (char *haystack, char *needle, int n);

#endif
