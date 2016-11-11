#include <stdio.h>
#include "global.hh"

char *infile_name = NULL;   // input file's name
char *dumpfile_name = NULL; // dump file's name
FILE *infp = NULL;          // input file's pointer, default is stdin
FILE *dumpfp = NULL;        // dump file's pointer
