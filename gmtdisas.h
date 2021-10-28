
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>

/*----------------------------------------------------------------------------*/
/* Local headers */

#include "version.h"
#include "help.h"
#include "ins.h"
#include "ihex.h"
#include "asm.h"
/*----------------------------------------------------------------------------*/
/* Globals */

uint32_t prog_stat;
  #define PROG_STAT_OFILE		    0x0004
uint32_t prog_mode;
  #define PROG_MODE_VERBOSE	  	0x0001
  #define PROG_MODE_REL0  		  0x0002
  #define PROG_MODE_IONAME      0x0004

ioreg *ioregtable;
int   ioreg_cnt;

/*----------------------------------------------------------------------------*/
/* Project source files */

#include "ihex.c"
#include "asm.c"

