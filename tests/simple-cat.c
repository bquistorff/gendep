#include <stdio.h>
#include "foo.h"

int main ()
{
  int c;
  FILE * f=fopen ("Makefile","r");
  while ((c = fgetc (f)) != EOF)
    fputc (c, stderr);
  return 0;
}
