/*
  syscall.c -- override open(2)

  (c) Han-Wen Nienhuys <hanwen@cs.uu.nl> 1998
  (c) Brian Quistorff <bquistorff@gmail.com> 2015
 */

#include <stdio.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syscall.h>
#include <asm/unistd.h>
#include <unistd.h>
#include "gendep.h"

/*
  This breaks if we hook gendep onto another library that overrides open(2).
  (zlibc comes to mind)
 */

static ssize_t
real_read(int fd, void * buf, size_t count){
  return(syscall(SYS_read, (fd), (buf), (count)));
}
static ssize_t
real_write(int fd, __const void * buf, size_t count){
  return(syscall(SYS_write, (fd), (buf), (count)));
}

static int
real_open (const char *fn, int flags, int mode){
  return (syscall(SYS_open, (fn), (flags), (mode)));
}

void
real_close (int fd){
  syscall(SYS_close, (fd));
}

int
real_rename (const char * oldname, const char * newname){
  return syscall(SYS_rename, (oldname), (newname));
}

ssize_t
__read(int fd, void * buf, size_t count){
  gendep__register_read (fd, buf, count);
  return(real_read(fd, buf, count));
}

ssize_t
__write(int fd, __const void * buf, size_t count){
  gendep__register_write (fd, buf, count);
  return(real_write(fd, buf, count));
}

int
__open (const char *fn, int flags, ...){
  int rv ;
  /*int exists_if_rw=0;*/
  va_list p;
  va_start (p,flags);
  
  /*if (flags& O_RDWR){
    FILE *fn_file;
    if ((fn_file = fopen(fn, "r"))){
      exists_if_rw=1;
      fclose(fn_file);
    }
  }*/
    
  rv = real_open (fn, flags, va_arg (p, int));
  if (rv >=0)
    gendep__register_open (fn, flags, rv/*, exists_if_rw*/);

  return rv;    
}

void
__close(int fd){
  gendep__register_close (fd);
  real_close(fd);
}

int
__rename(const char * oldname, const char * newname){
  gendep__register_rename (oldname, newname);
  return real_rename(oldname, newname);
}

ssize_t read (int fd, void * buf, size_t count) __attribute__ ((alias ("__read")));
ssize_t write(int fd, __const void * buf, size_t count) __attribute__ ((alias ("__write")));
int open (const char *fn, int flags, ...) __attribute__ ((alias ("__open")));
int close (int fd) __attribute__ ((alias ("__close")));
int rename (const char * oldname, const char * newname) __attribute__ ((alias ("__rename")));
