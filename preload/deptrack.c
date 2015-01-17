/*
  deptrack.c -- high-level routines for gendepend.

  (c) Han-Wen Nienhuys <hanwen@cs.uu.nl> 1998
  (c) Brian Quistorff <bquistorff@gmail.com> 2015
  
  Note: This LD_PRELOAD approach is not 100%
  1) Some executables make system calls directly without libC
  2) Some executables statically link their libC code so it's not in a library
  (see http://stackoverflow.com/questions/6238066/)
  This means that on some systems 'gcc -o prog prog.c' will not find that the
  executable file was written. For these situations one should probably use
  strace.
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "gendep.h"

#define STRLEN 1024


static int debug = 0;
static int pid;

extern int errno;

static FILE * read_deps;
static FILE * write_deps;
static int parent=0;
static struct{
  regex_t *re_array;
  char *invert_array;
  int max, no;
} regexps;

static struct {
  char **fn_array;
  int *fd_array;
  int max, no;
}tracked_files;

static char * target;


void
xnomem(void *p)
{
  if (!p){
    errno = ENOMEM;
    perror ("libgendep.so");
  }
}

static char *
xstrdup (char *s){
  char *c = strdup (s);
  xnomem (c);
  return c;
}

static void *
xrealloc (void *s, int sz){
  void *c = realloc (s, sz);
  xnomem (c);
  return c;
}

static void *
xmalloc (int sz){
  void *c;
  c = malloc (sz);
  xnomem (c);
  return c;
}

static int
gendep_getenv (char **dest, char *what){
  char var[STRLEN]="GENDEP_";
  strcat (var, what);
  *dest = getenv (var);

  return !! (*dest);
}

/*
  Do hairy stuff with regexps and environment variables.
  XXX not sure if I should free regexp_val after xstrdup
 */
static void
setup_regexps (void){
  char *regexp_val =0;
  char *end, *start;

  /*if (debug) printf("PART1\n", pid);	*/
  if (!gendep_getenv (&regexp_val, "FMATCH"))
    return;

  /*if (debug) printf("PART2 [%s]\n", regexp_val);	*/
  if (!gendep_getenv (&target, "TARGET"))
    return;

  /*if(debug)printf("PART3 [%s]\n", target);	*/

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

    
    if (*start){
      regex_t regex;
      int result= regcomp (&regex, start, REG_NOSUB);

      if (result){
        fprintf (stderr, "libgendep.so: Bad regular expression `%s\'\n", start);
        goto duh;		/* ugh */
      }

      if (regexps.no >= regexps.max){
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

static void initialize (void) __attribute__ ((constructor));

static void initialize (void){
  char *regexp_val =0;

  pid = getpid();
  debug=(gendep_getenv (&regexp_val, "DEBUG"));

  if (debug) printf("[%d] PART1\n", pid);
  setup_regexps ();

  if (regexps.re_array){
    char fn_read[STRLEN];
    char fn_write[STRLEN];
    
    strncpy (fn_read, target, STRLEN);
    strncat (fn_read, ".dep", STRLEN);
    strncpy (fn_write, target, STRLEN);
    strncat (fn_write, ".tmp.dep", STRLEN);
    
    /*Create and write the read header if it doesn't exist*/
    if (!(read_deps = fopen(fn_read, "r"))){
      char chunk[STRLEN];
      strncpy (chunk, target, STRLEN);
      strncat (chunk, " : ", STRLEN);
      parent=1;
      read_deps = fopen (fn_read, "w");
      fputs (chunk, read_deps);
    }
    fclose(read_deps);
    
    read_deps = fopen (fn_read, "a+");
    write_deps = fopen (fn_write, "a+");
    
    
    tracked_files.fn_array = xmalloc (0);
    tracked_files.fd_array = xmalloc (0);
    tracked_files.no=0;
    tracked_files.max=0;
  }
}

int
fn_in_file(char const *fn, FILE *file){
  char chunk[STRLEN];
  fseek( file, 0, SEEK_SET );
	while(fgets(chunk, STRLEN, file) != NULL) {
		if((strstr(chunk, fn)) != NULL) {
			return 1;
		}
	}
  return 0;
}

/* Someone is opening a file.  If it matches the regular expressions, write 
  it to the appropriate dep-file. 
  As long as the the appending string len is < PIPE_BUF then this is atomic.
  These open flags are mapped from fopen mode.
 */
void
gendep__register_open (char const *fn, int flags, int fd/*, int existed_if_rw*/){
  int i;
  
  if (!read_deps | !write_deps) {
    /*if (debug) printf("[%d] OpenMiss1 [%s] [%d] [%i [%i] %i]\n",pid, fn,fd, O_RDONLY, O_RDWR, O_WRONLY);*/
    return;
  }
  
  for (i =0; i<  regexps.no; i++){
    int not_matched = regexec (regexps.re_array +i, fn, 0, NULL, 0);

    if (!(not_matched ^ regexps.invert_array[i])){
      /*if (debug) printf("[%d] OpenMiss2 [%s] [%d] [%i [%i] %i]\n",pid, fn,fd, O_RDONLY, O_RDWR, O_WRONLY);*/
      return;
    }
  }
  
  if (debug) printf("[%d] Open [%s] [%d] [%i [%i] %i]\n",pid, fn,fd, O_RDONLY, O_RDWR, O_WRONLY);
  
  /* Put fd, fname in set*/
  /* Should I check if fn already in there? */
  if(tracked_files.no==tracked_files.max){
    tracked_files.max++;
    tracked_files.fd_array = xrealloc (tracked_files.fd_array, tracked_files.max * sizeof (int));
    tracked_files.fn_array = xrealloc (tracked_files.fn_array, tracked_files.max * sizeof (char *));
  }
  tracked_files.fd_array[tracked_files.no]=fd;
  tracked_files.fn_array[tracked_files.no++]=strdup(fn);
}

int ind_of_fd(int fd){
  int i;
  for(i=0; i<tracked_files.no; i++){
    if(fd==tracked_files.fd_array[i]){
      return i;
    }
  }
  return -1;
}

void gendep__register_close (int fd){
  int i, nleft;
  
  if((i = ind_of_fd(fd))==-1) return;
  nleft = tracked_files.no-i-1;
  if (debug) printf("[%d] Close [%s]\n",pid, tracked_files.fn_array[i]);
  /*remove fd from set*/
  free(tracked_files.fn_array[i]);
  if(nleft>0){
    memmove(&tracked_files.fn_array[i], &tracked_files.fn_array[i+1], sizeof(char *)*(nleft));
    memmove(&tracked_files.fd_array[i], &tracked_files.fd_array[i+1], sizeof(int)*(nleft));
  }
  tracked_files.no--;
}

void gendep__register_read (int fd, void * buf, size_t count){
  int i;
  char *fn;
  char chunk[STRLEN];

  if((i = ind_of_fd(fd))==-1) return;
  
  fn = tracked_files.fn_array[i];
  if (debug) printf("[%d] Read [%s]\n",pid, fn);
  if(!fn_in_file(fn, read_deps) && !fn_in_file(fn, write_deps)){
    strncpy (chunk, fn, STRLEN);
    strncat (chunk, " ", STRLEN);
    fputs (chunk, read_deps);
  }
}
void gendep__register_write (int fd, __const void * buf, size_t count){
  int i;
  char *fn;
  char chunk[STRLEN];
  
  if((i = ind_of_fd(fd))==-1) return;
  
  fn = tracked_files.fn_array[i];
  if (debug) printf("[%d] Write [%s]\n",pid, fn);
  if(!fn_in_file(fn, write_deps)){
    strncpy (chunk, fn, STRLEN);
    strncat (chunk, " ", STRLEN);
    fputs (chunk, write_deps);
  }
}


void gendep__register_rename (const char * oldname, const char * newname){
  if (debug) printf("[%d] Rename [%s] [%s]\n",pid, oldname, newname);
}

static void finish (void) __attribute__ ((destructor));

static void finish (void){
  char fn_write[STRLEN];
  int i;
  
  /* Free the regexp memory and then the file list memory */
  for(i=0; i<regexps.no; i++){
    regfree(&regexps.re_array[i]);
  }
  free(regexps.re_array);
  free(regexps.invert_array);
  
  for(i=0; i<tracked_files.no; i++){
    free(tracked_files.fn_array[i]);
  }
  free(tracked_files.fn_array);
  free(tracked_files.fd_array);
  
  if (debug) printf("[%d] Finishing [parent=%i, target=%s]\n",pid, parent, target);
  
  /* Try to put the two files together if parent*/
  if(write_deps) fclose(write_deps);
  if (!parent){
    if(read_deps) fclose(read_deps);
    return;
  }
  
  strncpy (fn_write, target, STRLEN);
  strncat (fn_write, ".tmp.dep", STRLEN);
  
  if (!read_deps){
    /*bail out*/
    if (write_deps){
      remove(fn_write);
    }
    return;
  }
  
  fputs ("\n", read_deps);
  
  if(write_deps && (write_deps = fopen (fn_write, "r"))){
    char chunk[STRLEN];
    i =0;
    while(fgets(chunk, STRLEN, write_deps) != NULL){
      fputs (chunk, read_deps);
      i++;
    }
    fclose(write_deps);
    
    if(i>0){
      fputs (" : ", read_deps);
      fputs (target, read_deps);
      fputs ("\n", read_deps);
    }
    remove(fn_write);
  }
  
  fclose (read_deps);
}

