/* Main source file for gmtdisas
 * Cristian Gyorgy, 2021 */


#include "gmtdisas.h"

static void
show_version (void)
{
  printf ("GmtDisas %s, STM8 dissassembler\n",
      SOFTWARE_VERSION);
}

static void
show_help (void)
{
  show_version ();
  puts (help_text);
}


int
main (int argc, char **argv)
{
  char  *hexfilename = NULL;
  char  *outfilename = NULL;
  FILE  *hexfile, *asmfile;

  prog_mode = 0;
  prog_stat = 0;
  
//check user arguments
  for (int i=1; i<argc; i++) {
    if ( !strcasecmp(argv[i], "-o") ) {
      i++;
      if (i>=argc) {
        printf ("Missing argument for -o option!\n");
        exit (EXIT_FAILURE);
      }
      outfilename = argv[i];
    } else if ( !strcasecmp(argv[i], "-rel0") ) {
      prog_mode |= PROG_MODE_REL0;
    } else if ( !strcasecmp(argv[i], "-ioname") ) {
      prog_mode |= PROG_MODE_IONAME;
    } else if ( !strcasecmp(argv[i], "-h") ) {
      show_help ();
    } else if ( !strcasecmp(argv[i], "--help") ) {
      show_help ();
    } else if ( !strcasecmp(argv[i], "--version") ) {
      show_version ();
    } else if (i==argc-1) {
      hexfilename = argv[i];
    } else {
    //unknown option/argument
      printf ("...Unknown argument \"%s\"\n", argv[i]);
      exit (EXIT_FAILURE);
    }
  }


  if (!hexfilename) {
    puts ("No input file!");
    exit (EXIT_FAILURE);
  }
  hexfile = fopen (hexfilename, "r");
  if (!hexfile) {
    puts (strerror(errno));
    exit (EXIT_FAILURE);
  }

  if (outfilename)
    asmfile = fopen (outfilename, "w+");
  else
    asmfile = fopen ("dissasembly.asm", "w+");
  if (!asmfile) {
    puts (strerror(errno));
    exit (EXIT_FAILURE);
  }

  if (prog_mode & PROG_MODE_IONAME) {
    FILE *iofile = fopen ("/usr/share/gmtdisas/stm8.inc", "r");
    if (!iofile) {
      puts (strerror(errno));
      prog_mode &= ~PROG_MODE_IONAME;
    }
    char linie[128];
    char namescan[128];
    ioreg_cnt = 0;
    uint32_t uk;
    
    //first we count the number of definitions for correct size allocation
    while ( fgets(linie, sizeof(linie), iofile) ) {
      if ( !strncmp(linie, ".equ ", 5)
          && (sscanf(linie+5, "%*s %X", &uk) == 1)
          && (uk >= 0x5000)
          && (uk < 0x5800) )
        ioreg_cnt++;
    }
    rewind (iofile);
    ioregtable = malloc (sizeof(ioreg)*ioreg_cnt);
    if (!ioregtable) {
      printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__, strerror(errno));
      exit (EXIT_FAILURE);
    }
    
    //now we scan the io definitions in ioregtable
    ioreg_cnt = 0;
    while ( fgets(linie, sizeof(linie), iofile) ) {
      if ( !strncmp(linie, ".equ ", 5)
          && (sscanf(linie+5, "%s %X", namescan, &uk) == 2)
          && (uk >= 0x5000)
          && (uk < 0x5800) ) {
        int i = strlen(namescan);
        
        if (namescan[i-1]==',') {
          namescan[i-1] = 0x00;
          namescan[31]  = 0x00;
          strncpy ( (ioregtable + ioreg_cnt)->name, namescan, 32);
          (ioregtable + ioreg_cnt)->add = uk;
          ioreg_cnt++;
        }
      }
    }
  }

  int q, area;
  long fpos;
  datablock db = {0, 0, 0, 0, NULL};
  datablock dd = {0, 0, 0, 0, NULL};

  area = AREA_NONE;
  do {    
    fpos = ftell(hexfile);
    q = Ihex_Read_Data_Block_Size (hexfile, &db);
    if (q>=0) {
      if (db.size > 0) {
        unsigned char *data = malloc (db.size);
        if (!data) {
          printf ("%s:%s:%i: %s\n", __FILE__, __func__, __LINE__,
             strerror(errno));
          exit (EXIT_FAILURE);
        }
      
        dd.data = data;
        dd.start_add = db.start_add;
        fseek(hexfile, fpos, SEEK_SET);

        if (Ihex_Read_Data_Block (hexfile, &dd) == -1) {
          puts("eerrororr");
          exit(EXIT_FAILURE);
        }

        if (dd.start_add < 0x4800) {
        //EEPROM data block
          if (area!=EEPROM) {
            area = EEPROM;
            fprintf (asmfile, ";-----------------------------------------------"
                "--------------------------------\n.area EEPROM (ABS, DSEG)\n");
          }
          Write_Eeprom_Data (asmfile, &dd);
        } else if (dd.start_add < 0x4880) {
        //OPT data block
          if (area!=OPT) {
            area = OPT;
            fprintf (asmfile, ";-----------------------------------------------"
                "--------------------------------\n.area OPT (ABS, DSEG)\n");
          }
          Write_Opt_Data (asmfile, &dd);
        } else if (dd.start_add >= 0x6000) {
          if (area!=FLASH) {
            area = FLASH;
            fprintf (asmfile, ";-----------------------------------------------"
                "--------------------------------\n.area FLASH (ABS, CSEG)\n");
          }
          Write_Code_Data (asmfile, &dd);
        } else {
        //GPIO
        }
        free(data);
      }
    }
  } while (q > 0);

  if (prog_mode & PROG_MODE_IONAME)
    free (ioregtable);
  fclose (hexfile);
  fclose (asmfile);

  exit(EXIT_SUCCESS);	
}

/* Fin */
