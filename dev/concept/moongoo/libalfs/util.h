#ifndef __UTIL_H__
#define __UTIL_H__

#include <stdbool.h>

/* Attributes */
#define RESET           0
#define BRIGHT          1
#define DIM             2
#define UNDERLINE       3
#define BLINK           4
#define REVERSE         7
#define HIDDEN          8

/* Colors */
#define BLACK           0
#define RED             1
#define GREEN           2
#define YELLOW          3
#define BLUE            4
#define MAGENTA         5
#define CYAN            6
#define WHITE           7

void term_set (int attr, int fg, int bg);
void term_reset ();

char *lower_case (char *str);
char *strdog (char *str1, char *str2);
char *strdog2 (char *str1, char *str2);
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
char *cut_trail (char *str, char *delim);
char *strdog_path (char *str1, char *str2);
char *extonly (char *url);

#endif
