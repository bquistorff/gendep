/*
  deptrack.c -- high-level routines for gendepend.

  (c) Han-Wen Nienhuys <hanwen@cs.uu.nl> 1998
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>

#include "gendep.h"


const int debug = 0;

extern int errno;

static FILE * output;
static struct
{
  regex_t *re_array;
  char *invert_array;
  int max, no;
} regexps;

static char * executable_name;
static char * wanted_executable_name;
static char * target;


#define STRLEN 1024

/*
  simplistic wordwrap
 */
void
write_word (char const * word)
{
  static int column;
  int l = strlen (word);
  if (l + column > 75)
    {
      fputs ("\\\n\t",output);
      column = 8;
    }

  column += l+1;
  fputs (word, output);
  fputs (" ", output);
}

void
xnomem(void *p)
{
  if (!p)
    {
      errno = ENOMEM;
      perror ("libgendep.so");
    }
}

static char *
xstrdup (char *s)
{
  char *c = strdup (s);
  xnomem (c);
  return c;
}

static void *
xrealloc (void *s, int sz)
{
  void *c = realloc (s, sz);
  xnomem (c);
  return c;
}

static void *
xmalloc (int sz)
{
  void *c;
  c = malloc (sz);
  xnomem (c);
  return c;
}

static int
gendep_getenv (char **dest, char *what)
{
  char var[STRLEN]="GENDEP_";
  strcat (var, what);
  *dest = getenv (var);

  return !! (*dest);
}

/*
  Do hairy stuff with regexps and environment variables.
 */
static void
setup_regexps (void)
{
  char *regexp_val =0;
  char *end, *start;

  if (debug) printf("PART1\n");	
  if (!gendep_getenv (&wanted_executable_name, "BINARY"))
    return;

  if (debug) printf("PART2 [%s] [%s]\n", wanted_executable_name, executable_name);		
  if (strcmp (wanted_executable_name, executable_name))
    {
      return;
    }

  if (debug) printf("PART3\n");	
  if (!gendep_getenv (&regexp_val, executable_name))
    return;

  if (debug) printf("PART4 [%s]\n", regexp_val);	
  if (!gendep_getenv (&target, "TARGET"))
    return;

  if(debug)printf("PART5 [%s]\n", target);	

  /*
    let's hope malloc (0) does the sensible thing (not return NULL, that is)
  */
  regexps.re_array = xmalloc (0);
  regexps.invert_array = xmalloc (0);
  
  regexp_val = xstrdup (regexp_val);
  start = regexp_val;
  end = regexp_val + strlen (regexp_val);

  /*
    Duh.  The strength of C : string handling. (not).   Pull apart the whitespace delimited
    list of regexps, and try to compile them.
    Be really gnuish with dynamic allocation of arrays. 
   */
  do {
    char * end_re = 0;
    int in_out;
    while (*start && *start != '+' && *start != '-')
      start ++;

    in_out= (*start == '+') ;
    start ++;
    end_re = strchr (start, ' ');
    if (end_re)
      *end_re = 0;

    
    if (*start)
      {
	regex_t regex;
	int result= regcomp (&regex, start, REG_NOSUB);

	if (result)
	  {
	    fprintf (stderr, "libgendep.so: Bad regular expression `%s\'\n", start);
	    goto duh;		/* ugh */
	  }

	if (regexps.no >= regexps.max)
	  {
	    /* max_regexps = max_regexps * 2 + 1; */
	    regexps.max ++;
	    regexps.re_array = xrealloc (regexps.re_array, regexps.max * sizeof (regex_t));
	    regexps.invert_array = xrealloc (regexps.invert_array, regexps.max * sizeof (char));	
	  }

	regexps.re_array[regexps.no] = regex;
	regexps.invert_array[regexps.no++] = in_out;
      }

  duh:
    start = end_re ? end_re + 1 :  end;
  } while (start < end);
}

/*
  Try to get the name of the binary.  Is there a portable way to do this?
 */
static void get_executable_name (void)
{
  FILE *cmdline = fopen ("/proc/self/cmdline", "r");
  char cmd[STRLEN];
  int i=0;
  char *basename_p;
  int c;
  cmd[STRLEN-1] = 0;
  
  while ((c = fgetc(cmdline))!=EOF && c && i < STRLEN-1)
    cmd[i++] = c;

  cmd[i++] = 0;

  /* ugh.  man 3 basename -> ?  */
  basename_p =  strrchr (cmd, '/');
  if (basename_p)
    basename_p++;
  else
    basename_p = cmd;

  executable_name = xstrdup (basename_p);
}

static void initialize (void) __attribute__ ((constructor));

static void initialize (void)
{
  get_executable_name ();
  setup_regexps ();

  if (regexps.re_array)
    {
      char fn[STRLEN];
      if (target)
	strncpy (fn, target, STRLEN);
      else
	strncpy (fn, wanted_executable_name, STRLEN);
      
      strncat (fn, ".dep", STRLEN);
      output = fopen (fn, "w");

      write_word (target);
      write_word (":");
    }
}

/* Someone is opening a file.  If it is opened for reading, and
  matches the regular expressions, write it to the dep-file. 
 */
void
gendep__register_open (char const *fn, int flags)
{
  if (debug) printf("FN [%s] %p %i %i %i\n",fn,(void*)output,flags, O_WRONLY, O_RDWR);
  if (output && !(flags & (O_WRONLY | O_RDWR)))
    {
      int i;
      for (i =0; i<  regexps.no; i++)
	{
	  int not_matched = regexec (regexps.re_array +i, fn, 0, NULL, 0);

	  if (!(not_matched ^ regexps.invert_array[i]))
	    return;
	}

      write_word (fn);
    }
}

static void finish (void) __attribute__ ((destructor));

static void finish (void)
{
  if (output)
    {
      fprintf (output, "\n");
      fclose (output);
    }
}

