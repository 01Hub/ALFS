#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <util.h>

char *lower_case (char *arg)
{
	char *str;
	int i;
	
	if (!arg)
		return NULL;

	str = (char *)malloc(strlen(arg)+1);
	strcpy(str, arg);

	for (i=0;i<strlen(str);i++)
		if (isalpha(str[i]))
			str[i]=tolower(str[i]);

	return str;
}

char *strdog (char *str1, char *str2)
{
	char *ret;

	if ((!str1)||(!str2))
		return NULL;
		
	ret=(char *)malloc(strlen(str1)+strlen(str2)+1);

	strcpy(ret, str1);
	strcat(ret, str2);
	return ret;
}

char *strdog2 (char *str1, char *str2)
{
	char *ret;

	if ((!str1)||(!str2))
		return NULL;
	
	ret=(char *)malloc(strlen(str1)+strlen(str2)+2);

	strcpy(ret, str1);
	if (strlen(str1)>0)
		strcat(ret, " ");
	strcat(ret, str2);
	free(str1);
	return ret;
}

char *chrep (char *str, char old, char new)
{
	char *ret;
	int i;

	if (!str)
		return NULL;
	
	ret=(char *)malloc(strlen(str)+1);

	for (i=0;i<strlen(str);i++)
		if (str[i]==old)
			ret[i]=new;
		else
			ret[i]=str[i];
	ret[i]='\0';

	return ret;
}

char **tokenize (char *arg, char *token, int *i)
{
	int n=0;
	char **ret = NULL, *str;

	if ((!arg)||(!token))
		return NULL;

	str=(char *)malloc(strlen(arg)+1);
	strcpy(str, arg);

	while (strcnt(str, " "))
	{
		ret = realloc(ret, (++n)*sizeof(char *));
		ret[n-1] = strcut(str, 0, whereis(str, ' '));
		str = notrail(strstr(str, token), token);
	}

	ret = realloc(ret, (++n)*sizeof(char *));
	ret[n-1] = (char *)malloc(strlen(str)+1);
	strcpy(ret[n-1], str);
	*i = n;
	return ret;
}

char *strrep (char *str, char *old, char *new)
{
	char *ret;
	int i, j=0, k;

	if ((!str)||(!old)||(!new))
		return NULL;
	
	ret=(char *)malloc(strlen(str)+1+((strlen(new)-strlen(old))*
			strcnt(str, old)));

	for (i=0;i<strlen(str);i++)
		if (strstarts(str, i, old))
		{
			for (k=j;k<j+strlen(new);k++)
				ret[k]=new[k-j];
			i+=strlen(old)-1;
			j+=strlen(new);
		}
		else
		{
			ret[j]=str[i];
			j++;
		}
	ret[j]='\0';

	return ret;
}

int strcnt (char *str, char *sstr)
{
	char *t, *t2;
	int i, ret=0;

	if ((!str)||(!sstr))
		return 0;

	t=(char *)malloc(strlen(str)+1);
	strcpy(t, str);
	t2=t;
	while (t)
	{
		t=strstr(t, sstr);
		if (t)
		{
			ret++;
			for (i=0;i<strlen(sstr);i++)
				t++;
		}
	}

	free(t2);
	return ret;
}

int strstarts (char *str, int begin, char *test)
{
	char *tmp;
	int ret=0;

	if ((!str)||(strlen(str)<begin+strlen(test)))
		return 0;

	tmp=strcutb(str, begin, strlen(test), false);

	if (!strcmp(tmp, test))
		ret=1;

	free(tmp);
	return ret;
}

char *strcutb (char *str, int begin, int count, bool blanks)
{
	int i, j=0, end;
	char *ret;
	
	if (!str)
		return NULL;
	if (count<0)
		count=0;
	
	ret=(char *)malloc(count+1);

	if (count<1)
	{
		strcpy(ret, "");
		return ret;
	}

	if (strlen(str)<begin+count)
		end=strlen(str);
	else
		end=begin+count;
	
	for (i=begin;i<end;i++)
		if (!((j==0)&&(blanks)&&(str[i]==' ')))
		{
			ret[j]=str[i];
			j++;
		}
	ret[j]='\0';

	return ret;
}

char *strcut (char *str, int begin, int count)
{
	return strcutb(str, begin, count, true);
}

int whereis (char *str, char ch)
{
	int i;

	if (!str)
		return -1;
	
	for (i=0;i<strlen(str);i++)
		if (str[i]==ch)
			return i;

	return -1;
}

char *notrail (char *str, char *token)
{
	char *ret;
	
	if ((!str)||(!token))
		return NULL;

	ret=(char *)malloc(strlen(str)+1);
	strcpy(ret, str);
	while (strstarts(ret, 0, token))
		ret+=strlen(token);
	return ret;
}

char *squeeze (char *str)
{
	bool blank=false;
	int i, len=0;
	char *ret;

	if (!str)
		return NULL;

	ret=(char *)malloc(strlen(str)+1);

	for (i=0;i<strlen(str);i++)
	{
		if (isblank(str[i]))
		{
			if (blank)
				continue;
			blank=true;
		}
		else
			blank=false;
		
		ret[len] = str[i];
		len++;
	}
	
	ret[len]='\0';
	return ret;
}

char *strkill (char *str, char *tokill)
{
	char *cpy;
	int i, j=0;

	if ((!str)||(!tokill))
		return NULL;
	
	cpy=(char *)malloc(strlen(str)+1);
	
	for (i=0;i<strlen(str);i++)
		if (strstarts(str, i, tokill))
			i+=strlen(tokill)-1;
		else
		{
			cpy[j]=str[i];
			j++;
		}
	cpy[j]='\0';

	return cpy;
}

char *strnstr (char *haystack, char *needle, int n)
{
	int i;
	char *ret = haystack;

	if ((!haystack)||(!needle))
		return NULL;
	
	for (i=0;i<n;i++)
	{
		ret = strstr(ret, needle);
		if (strcmp(haystack, ret))
			ret++;
	}

	return ret;
}	

void term_set (int attr, int fg, int bg)
{
	char cmd[13];

	sprintf(cmd, "%c[%d;%d;%dm", 0x1B, attr, fg+30, bg+40);
	printf(cmd);
}

void term_reset ()
{
	char cmd[13];

	sprintf(cmd, "%c[m", 0x1B);
	printf(cmd);
}

char *cut_trail (char *str, char *delim)
{
	char *ret;

	if (!str)
		return NULL;
	
	ret = (char *)malloc(1);
	strcpy(ret, "");
		
	while ((str) && (strlen(str)))
	{
		char *t = ret, *tmp = strsep(&str, "\n");
		tmp = notrail(tmp, "\t");
		
		if (!tmp)
			continue;
		
		if (strcmp(tmp, ""))
		{
			tmp = strstr(tmp, delim);
			tmp++;
		}

		ret = strdog(ret, tmp);
		free(t);
		t = ret;
		ret = strdog(ret, "\n");
		free(t);
	}

	return ret;
}
