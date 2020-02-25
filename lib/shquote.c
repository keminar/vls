#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "shquote.h"

/*
 思路来自 https://blog.csdn.net/liuqz2009/article/details/8776398
 源码来自 http://ftp.gnu.org/gnu/bash/bash-4.2.tar.gz
*/


/* **************************************************************** */
/*								    */
/*	 Functions for quoting strings to be re-read as input	    */
/*								    */
/* **************************************************************** */

/* Return a new string which is the single-quoted version of STRING.
   Used by alias and trap, among others. */
char *
sh_single_quote (string)
     const char *string;
{
  register int c;
  char *result, *r;
  const char *s;

  result = (char *)malloc (3 + (4 * strlen (string)));
  r = result;

  if (string[0] == '\'' && string[1] == 0)
    {
      *r++ = '\\';
      *r++ = '\'';
      *r++ = 0;
      return result;
    }

  *r++ = '\'';

  for (s = string; s && (c = *s); s++)
    {
      *r++ = c;

      if (c == '\'')
	{
	  *r++ = '\\';	/* insert escaped single quote */
	  *r++ = '\'';
	  *r++ = '\'';	/* start new quoted string */
	}
    }

  *r++ = '\'';
  *r = '\0';

  return (result);
}


/* Quote special characters in STRING using backslashes.  Return a new
   string.  NOTE:  if the string is to be further expanded, we need a
   way to protect the CTLESC and CTLNUL characters.  As I write this,
   the current callers will never cause the string to be expanded without
   going through the shell parser, which will protect the internal
   quoting characters. */
char *
sh_backslash_quote (string)
     char *string;
{
  int c;
  char *result, *r, *s;

  result = (char *)malloc (2 * strlen (string) + 1);

  for (r = result, s = string; s && (c = *s); s++)
    {
      switch (c)
	{
	case ' ': case '\t': case '\n':		/* IFS white space */
	case '\'': case '"': case '\\':		/* quoting chars */
	case '|': case '&': case ';':		/* shell metacharacters */
	case '(': case ')': case '<': case '>':
	case '!': case '{': case '}':		/* reserved words */
	case '*': case '[': case '?': case ']':	/* globbing chars */
	case '^':
	case '$': case '`':			/* expansion chars */
	case ',':				/* brace expansion */
	  *r++ = '\\';
	  *r++ = c;
	  break;

	case '~':				/* tilde expansion */
	  if (s == string || s[-1] == '=' || s[-1] == ':')
	    *r++ = '\\';
	  *r++ = c;
	  break;

	case CTLESC: case CTLNUL:		/* internal quoting characters */
	  *r++ = CTLESC;			/* could be '\\'? */
	  *r++ = c;
	  break;


	case '#':				/* comment char */
	  if (s == string)
	    *r++ = '\\';
	  /* FALLTHROUGH */
	default:
	  *r++ = c;
	  break;
	}
    }

  *r = '\0';
  return (result);
}


int
sh_contains_shell_metas (string)
     char *string;
{
  char *s;

  for (s = string; s && *s; s++)
    {
      switch (*s)
	{
	case ' ': case '\t': case '\n':		/* IFS white space */
	case '\'': case '"': case '\\':		/* quoting chars */
	case '|': case '&': case ';':		/* shell metacharacters */
	case '(': case ')': case '<': case '>':
	case '!': case '{': case '}':		/* reserved words */
	case '*': case '[': case '?': case ']':	/* globbing chars */
	case '^':
	case '$': case '`':			/* expansion chars */
	  return (1);
	case '~':				/* tilde expansion */
	  if (s == string || s[-1] == '=' || s[-1] == ':')
	    return (1);
	  break;
	case '#':
	  if (s == string)			/* comment char */
	    return (1);
	  /* FALLTHROUGH */
	default:
	  break;
	}
    }

  return (0);
}
