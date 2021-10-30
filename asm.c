
int
Get_IOreg_Name (uint32_t add, char *text) {
  if (!(prog_mode & PROG_MODE_IONAME))
    return -1;

  for (int i=0; i<ioreg_cnt; i++)
    if ( (ioregtable+i)->add == add ) {
      strcpy (text, (ioregtable+i)->name);
      return 0;
    }

  return -1;
}

/*
 * EEPROM: [0x1000:0x17FF) sau [0x4000:0x4800)
 * //EEPROM data block
 * ;0xQQPP
 * ;  .byte 0xAA,0xBB,
 */
void
Write_Eeprom_Data (FILE *file, datablock *block) {
  int       bytes = 0;
  uint32_t  eep_add = block->start_add;

  fprintf (file, ";-----------------------------------------------"
                "--------------------------------\n");
  fprintf (file, ".org 0x%04X\n", block->start_add);

  while ((bytes < block->size) && (eep_add <= 0x4800)) {
    if ((bytes % 8) == 0) {
      fprintf(file, "\n;0x%04X\n\t.byte 0x%02X", eep_add, block->data[bytes++]);
      eep_add += 8;
    } else {
        fprintf (file, ",0x%02X", block->data[bytes++]);
    }
  }

  fprintf (file, "\n");
}

/*
 * Opt. bytes: [0x4800:0x4880)
 * //OPT data bytes
 * ;0xQQPP
 * ;  .byte 0xAA,0xBB,
 */
void
Write_Opt_Data (FILE *file, datablock *block) {
  int       bytes = 0;
  uint32_t  opt_add = block->start_add;

  fprintf (file, ";-----------------------------------------------"
                "--------------------------------\n");
  fprintf (file, ".org 0x%04X\n", block->start_add);

  while ( (bytes < block->size) && (opt_add <= 0x4880) ) {
    if ((bytes % 4) == 0) {
      fprintf(file, "\n;0x%04X\n\t.byte 0x%02X", opt_add, block->data[bytes++]);
      opt_add += 4;
    } else {
        fprintf (file, ",0x%02X", block->data[bytes++]);
    }
  }

  fprintf (file, "\n");
}


/*
 *
 */
void
Write_Code_Data (FILE *file, datablock *block) {
  int cnt, n, add, err, wrno;
  instruction ins;
  int oc[6];
  char ioname[36];

  cnt = 0;
  add = block->start_add;

  fprintf (file, ";-----------------------------------------------"
                "--------------------------------\n");
  fprintf (file, ".org 0x%06X\n", block->start_add);

  while (cnt < block->size) {
    oc[0] = *(block->data + cnt);
    oc[1] = -1;
    oc[2] = -1;
    oc[3] = -1;
    oc[4] = -1;
    oc[5] = -1;

    err  = 0;
    wrno = 0;
    n    = 1;

    switch (oc[0]) {
      case 0x72:
        oc[1] = *(block->data + cnt + 1);
        ins = ins_table_72[oc[1]];
        n = 2;
        break;
      case 0x90:
        oc[1] = *(block->data + cnt + 1);
        ins = ins_table_90[oc[1]];
        n = 2;
        break;
      case 0x91:
        oc[1] = *(block->data + cnt + 1);
        if ((oc[1] >= 0x60) && (oc[1] <= 0xDF))
          ins = ins_table_91_0x60[oc[1] - 0x60];
        else
          err = 1;
        n = 2;
        break;
      case 0x92:
        oc[1] = *(block->data + cnt + 1);
        if ((oc[1] >= 0x30) && (oc[1] <= 0xDF))
          ins = ins_table_92_0x30[oc[1] - 0x30];
        else
          err = 1;
        n = 2;
        break;
      default:
        oc[1] = oc[0];
        oc[0] = -1;
        ins = ins_table[oc[1]];
    }

    if (err || !ins.size) {
      wrno += fprintf (file, "            .byte 0x%02X", oc[1]);
      ins.size = 1;
    } else {
      wrno += fprintf (file, "            %s", ins.text);

      if (n == 1) {
        for (; n<ins.size; n++)
          oc[n+1] = *(block->data + cnt + n);
      } else {
        for (; n<ins.size; n++)
          oc[n] = *(block->data + cnt + n);
      }

      switch (ins.des) {
        case NONE:
          break;
        case REG_A:
          wrno += fprintf (file, " A");
          break;
        case REG_XL:
          wrno += fprintf (file, " XL");
          break;
        case REG_YL:
          wrno += fprintf (file, " YL");
          break;
        case REG_XH:
          wrno += fprintf (file, " XH");
          break;
        case REG_YH:
          wrno += fprintf (file, " YH");
          break;
        case REG_CC:
          wrno += fprintf (file, " CC");
          break;
        case REG_X:
          wrno += fprintf (file, " X");
          break;
        case REG_Y:
          wrno += fprintf (file, " Y");
          break;
        case REG_SP:
          wrno += fprintf (file, " SP");
          break;
        case IMM_BYTE_2:
          wrno += fprintf (file, " #0x%02X", oc[2]);
          break;
        case IMM_WORD_23:
          wrno += fprintf (file, " #0x%02X%02X", oc[2], oc[3]);
          break;
        case PTR_X:
          wrno += fprintf (file, " (X)");
          break;
        case PTR_Y:
          wrno += fprintf (file, " (Y)");
          break;
        case SHORTMEM_2:
          wrno += fprintf (file, " 0x%02X", oc[2]);
          break;
        case SHORTMEM_3:
          wrno += fprintf (file, " 0x%02X", oc[3]);
          break;
        case LONGMEM_23:
          if (prog_mode & PROG_MODE_IONAME) {
            n = oc[2]<<8 | oc[3];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+1) ) {
              ioname[0] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, " 0x%02X%02X", oc[2], oc[3]);
            }
          } else {
            wrno += fprintf (file, " 0x%02X%02X", oc[2], oc[3]);
          }
          break;
        case LONGMEM_34:
          if (prog_mode & PROG_MODE_IONAME) {
            n = oc[3]<<8 | oc[4];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+1) ) {
              ioname[0] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, " 0x%02X%02X", oc[3], oc[4]);
            }
          } else {
            wrno += fprintf (file, " 0x%02X%02X", oc[3], oc[4]);
          }
          break;
        case LONGMEM_45:
          if (prog_mode & PROG_MODE_IONAME) {
            n = oc[4]<<8 | oc[5];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+1) ) {
              ioname[0] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, " 0x%02X%02X", oc[4], oc[5]);
            }
          } else {
            wrno += fprintf (file, " 0x%02X%02X", oc[4], oc[5]);
          }
          break;
        case EXTMEM_234:
          wrno += fprintf (file, " 0x%02X%02X%02X", oc[2], oc[3], oc[4]);
          break;
        case SHORTOFF_2:
          (oc[2] & 0x80) ? (n = oc[2] - 0x100) : (n = oc[2]);
          wrno += fprintf (file, " .%+-4i ;(0x%06X)",
              (prog_mode & PROG_MODE_REL0) ? (n+ins.size) : n,
              add + ins.size + n);
          break;
        case SHORTOFF_4:
          (oc[4] & 0x80) ? (n = oc[4] - 0x100) : (n = oc[4]);
          wrno += fprintf (file, " .%+-4i ;(0x%06X)",
              (prog_mode & PROG_MODE_REL0) ? (n+ins.size) : n,
              add + ins.size + n);
          break;
        case SHORTOFF_X_2:
          wrno += fprintf (file, " (0x%02X,X)", oc[2]);
          break;
        case SHORTOFF_Y_2:
          wrno += fprintf (file, " (0x%02X,Y)", oc[2]);
          break;
        case SHORTOFF_SP_2:
          wrno += fprintf (file, " (0x%02X,SP)", oc[2]);
          break;
        case LONGOFF_X_23:
          wrno += fprintf (file, " (0x%02X%02X,X)", oc[2], oc[3]);
          break;
        case LONGOFF_Y_23:
          wrno += fprintf (file, " (0x%02X%02X,Y)", oc[2], oc[3]);
          break;
        case EXTOFF_X_234:
          wrno += fprintf (file, " (0x%02X%02X%02X,X)", oc[2], oc[3], oc[4]);
          break;
        case EXTOFF_Y_234:
          wrno += fprintf (file, " (0x%02X%02X%02X,Y)", oc[2], oc[3], oc[4]);
          break;
        case SHORTPTR_2:
          wrno += fprintf (file, " [0x%02X]", oc[2]);
          break;
        case LONGPTR_23:
          wrno += fprintf (file, " [0x%02X%02X]", oc[2], oc[3]);
          break;
        case SHORTPTR_OFF_X_2:
          wrno += fprintf (file, " ([0x%02X],X)", oc[2]);
          break;
        case SHORTPTR_OFF_Y_2:
          wrno += fprintf (file, " ([0x%02X],Y)", oc[2]);
          break;
        case LONGPTR_OFF_X_23:
          wrno += fprintf (file, " ([0x%02X%02X],X)", oc[2], oc[3]);
          break;
        case LONGPTR_OFF_Y_23:
          wrno += fprintf (file, " ([0x%02X%02X],Y)", oc[2], oc[3]);
          break;
        case LONGMEM_BIT_123:
          if (prog_mode & PROG_MODE_IONAME) {
            n = oc[2]<<8 | oc[3];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+1) ) {
              ioname[0] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, " 0x%02X%02X", oc[2], oc[3]);
            }
          } else {
            wrno += fprintf (file, " 0x%02X%02X", oc[2], oc[3]);
          }
          wrno += fprintf (file, ", #%d", (oc[1] & 0x0F)>>1);
          break;
      }
      switch (ins.src) {
        case NONE:
          break;
        case REG_A:
          wrno += fprintf (file, ", A");
          break;
        case REG_XL:
          wrno += fprintf (file, ", XL");
          break;
        case REG_YL:
          wrno += fprintf (file, ", YL");
          break;
        case REG_XH:
          wrno += fprintf (file, ", XH");
          break;
        case REG_YH:
          wrno += fprintf (file, ", YH");
          break;
        case REG_CC:
          wrno += fprintf (file, ", CC");
          break;
        case REG_X:
          wrno += fprintf (file, ", X");
          break;
        case REG_Y:
          wrno += fprintf (file, ", Y");
          break;
        case REG_SP:
          wrno += fprintf (file, ", SP");
          break;
        case IMM_BYTE_2:
          wrno += fprintf (file, ", #0x%02X", oc[2]);
          break;
        case IMM_WORD_23:
          wrno += fprintf (file, ", #0x%02X%02X", oc[2], oc[3]);
          break;
        case PTR_X:
          wrno += fprintf (file, ", (X)");
          break;
        case PTR_Y:
          wrno += fprintf (file, ", (Y)");
          break;
        case SHORTMEM_2:
          wrno += fprintf (file, ", 0x%02X", oc[2]);
          break;
        case SHORTMEM_3:
          wrno += fprintf (file, ", 0x%02X", oc[3]);
          break;
        case LONGMEM_23:
         if (prog_mode & PROG_MODE_IONAME) {
            n = oc[2]<<8 | oc[3];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+2) ) {
              ioname[0] = ',';
              ioname[1] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, ", 0x%02X%02X", oc[2], oc[3]);
            }
          } else {
            wrno += fprintf (file, ", 0x%02X%02X", oc[2], oc[3]);
          }
          break;
        case LONGMEM_34:
         if (prog_mode & PROG_MODE_IONAME) {
            n = oc[3]<<8 | oc[4];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+2) ) {
              ioname[0] = ',';
              ioname[1] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, ", 0x%02X%02X", oc[3], oc[4]);
            }
          } else {
            wrno += fprintf (file, ", 0x%02X%02X", oc[3], oc[4]);
          }
          break;
        case LONGMEM_45:
         if (prog_mode & PROG_MODE_IONAME) {
            n = oc[4]<<8 | oc[5];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+2) ) {
              ioname[0] = ',';
              ioname[1] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, ", 0x%02X%02X", oc[4], oc[5]);
            }
          } else {
            wrno += fprintf (file, ", 0x%02X%02X", oc[4], oc[5]);
          }
          break;
        case EXTMEM_234:
          wrno += fprintf (file, ", 0x%02X%02X%02X", oc[2], oc[3], oc[4]);
          break;
        case SHORTOFF_2:
          (oc[2] & 0x80) ? (n = oc[2] - 0x100) : (n = oc[2]);
          wrno += fprintf (file, ", .%+-4i ;(0x%06X)",
              (prog_mode & PROG_MODE_REL0) ? (n+ins.size) : n,
              add + ins.size + n);
          break;
        case SHORTOFF_4:
          (oc[4] & 0x80) ? (n = oc[4] - 0x100) : (n = oc[4]);
          wrno += fprintf (file, ", .%+-4i ;(0x%06X)",
              (prog_mode & PROG_MODE_REL0) ? (n+ins.size) : n,
              add + ins.size + n);
          break;
        case SHORTOFF_X_2:
          wrno += fprintf (file, ", (0x%02X,X)", oc[2]);
          break;
        case SHORTOFF_Y_2:
          wrno += fprintf (file, ", (0x%02X,Y)", oc[2]);
          break;
        case SHORTOFF_SP_2:
          wrno += fprintf (file, ", (0x%02X,SP)", oc[2]);
          break;
        case LONGOFF_X_23:
          wrno += fprintf (file, ", (0x%02X%02X,X)", oc[2], oc[3]);
          break;
        case LONGOFF_Y_23:
          wrno += fprintf (file, ", (0x%02X%02X,Y)", oc[2], oc[3]);
          break;
        case EXTOFF_X_234:
          wrno += fprintf (file, ", (0x%02X%02X%02X,X)", oc[2], oc[3], oc[4]);
          break;
        case EXTOFF_Y_234:
          wrno += fprintf (file, ", (0x%02X%02X%02X,Y)", oc[2], oc[3], oc[4]);
          break;
        case SHORTPTR_2:
          wrno += fprintf (file, ", [0x%02X]", oc[2]);
          break;
        case LONGPTR_23:
          wrno += fprintf (file, ", [0x%02X%02X]", oc[2], oc[3]);
          break;
        case SHORTPTR_OFF_X_2:
          wrno += fprintf (file, ", ([0x%02X],X)", oc[2]);
          break;
        case SHORTPTR_OFF_Y_2:
          wrno += fprintf (file, ", ([0x%02X],Y)", oc[2]);
          break;
        case LONGPTR_OFF_X_23:
          wrno += fprintf (file, ", ([0x%02X%02X],X)", oc[2], oc[3]);
          break;
        case LONGPTR_OFF_Y_23:
          wrno += fprintf (file, ", ([0x%02X%02X],Y)", oc[2], oc[3]);
          break;
        case LONGMEM_BIT_123:
          if (prog_mode & PROG_MODE_IONAME) {
            n = oc[2]<<8 | oc[3];
            if ( (n>=0x5000) && (n<0x5800) && !Get_IOreg_Name(n, ioname+2) ) {
              ioname[0] = ',';
              ioname[1] = ' ';
              wrno += fprintf (file, ioname);
            } else {
              wrno += fprintf (file, ", 0x%02X%02X", oc[2], oc[3]);
            }
          } else {
            wrno += fprintf (file, ", 0x%02X%02X", oc[2], oc[3]);
          }
          wrno += fprintf (file, ", #%d", (oc[1] & 0x0F)>>1);
          break;
      }
    }

    for (; wrno<51; wrno++)
      fputc (' ', file);
    fprintf (file, ";0x%06X, ", add);
    for (n=0; n<6; n++) {
      if (oc[n]<0)
        fprintf (file, "   ");
      else
        fprintf (file, "%02X ", oc[n]);
    }
    fputc ('\n', file);


    cnt += ins.size;
    add += ins.size;
  }
}
