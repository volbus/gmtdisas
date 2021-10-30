
static int
read_next_byte (unsigned char *buffer, int offset)
{
  int qh, ql;

  qh = *(buffer+offset);
  ql = *(buffer+offset+1);

  if ( qh<0x3A && qh>0x2F ) {
    qh -= 0x30;
  } else {
    switch (qh) {
    case 'f':
    case 'F':
      qh = 0xF;
      break;
    case 'e':
    case 'E':
      qh = 0xE;
      break;
    case 'd':
    case 'D':
      qh = 0xD;
      break;
    case 'c':
    case 'C':
      qh = 0xC;
      break;
    case 'b':
    case 'B':
      qh = 0xB;
      break;
    case 'a':
    case 'A':
      qh = 0xA;
      break;
    default:
      return -1;
    }
  }

  if ( ql<0x3A && ql>0x2F ) {
    ql -= 0x30;
  } else {
    switch (ql) {
    case 'f':
    case 'F':
      ql = 0xF;
      break;
    case 'e':
    case 'E':
      ql = 0xE;
      break;
    case 'd':
    case 'D':
      ql = 0xD;
      break;
    case 'c':
    case 'C':
      ql = 0xC;
      break;
    case 'b':
    case 'B':
      ql = 0xB;
      break;
    case 'a':
    case 'A':
      ql = 0xA;
      break;
    default:
      return -1;
    }
  }

  return (qh<<4)|ql;
}

static int
read_next_word (unsigned char *buffer, int offset)
{
  int h, l;

  h = read_next_byte (buffer, offset);
  if (h==-1)
    return -1;
  l = read_next_byte (buffer, offset + 2);
  if (l==-1)
    return -1;
  return (h<<8)|l;
}


/*
 * It reads the next data block from the hex file and returns 1 for a non null
 * data block, -1 in case of error or 0 for end of file.
 * In the datablock structure writes the start_add, extoff_add, size and
 * line_sto fields.
 */
int
Ihex_Read_Data_Block_Size (FILE *file, datablock *block)
{
  unsigned char line[512+32];
  int           cnt, add, rec;

  long pos     = ftell (file);
  int  crtline = block->line_index;
  int  off     = block->ext_offset;
  int  blkadd  = 0;
  int  blksize = 0;


  while ( fgets((char *)line, sizeof(line), file) ) {
    crtline++;

    //check first char ':'
    if (line[0]!=':')
      goto file_err;

    //read the number of data bytes in the record
    cnt = read_next_byte (line, 1);
    if (cnt == -1)
        goto file_err;

    //read the address of the record
    add = read_next_word (line, 3);
    if (add == -1)
        goto file_err;

    //read the record type
    rec = read_next_byte (line, 7);
    if (rec==-1)
        goto file_err;

    switch (rec) {
    case 0x00:
    //data record
      if (cnt == 0)
        goto file_err;
      add += off;
      if (add != (blkadd + blksize)) {
          if (blkadd == 0) {
            blkadd = add;
          } else {
            crtline--;
            block->start_add  = blkadd;
            block->size       = blksize;
            block->line_index = crtline;
            fseek(file, pos, SEEK_SET);
            return 1;
          }
      }
      blksize += cnt;
      break;
    case 0x01:
    //end of file record
      if (cnt)
        goto file_err;
      block->start_add  = blkadd;
      block->size       = blksize;
      block->line_index = crtline;
      return 0;
      break;
    case 0x02:
    //extended address
      if (cnt != 2)
        goto file_err;
      off = read_next_word (line, 9);
      if (off == -1)
        goto file_err;
      off *= 16;
      block->ext_offset = off;
      break;
    default:
      printf ("Not supported record type found in hex file, line #%d\n",
          crtline);
      return -1;
    }

    pos = ftell(file);
  }

file_err:
/* In case of error we don't need to save any data back in the datablock
 * structure
 */
  printf ("Error in hex file, line #%d\n", crtline);
  return -1;
}



/*
 * ...
 */
int
Ihex_Read_Data_Block (FILE *file, datablock *block)
{
  uint32_t      chksum;
  unsigned char line[512+32];
  int           i, cnt, add, rec;

  long pos     = ftell (file);
  int  crtline = block->line_index;
  int  off     = block->ext_offset;
  int  blkadd  = block->start_add;
  int  blksize = 0;


  while ( fgets((char *)line, sizeof(line), file) ) {
    crtline++;
    i = 1;

    //read the number of data bytes in the record
    cnt = read_next_byte (line, i);
    if (cnt == -1)
        goto file_err;
    i += 2;
    chksum = cnt;

    //read the address of the record
    add = read_next_word (line, i);
    if (add == -1)
        goto file_err;
    i += 4;
    chksum += (add>>8) + (add & 0xFF);

    //read the record type
    rec = read_next_byte (line, i);
    if (rec==-1)
        goto file_err;
    i += 2;
    chksum += rec;

    switch (rec) {
    case 0x00:
    //data record
      add += off;
      if (add != (blkadd + blksize)) {
          crtline--;
          block->line_index = crtline;
          block->size = blksize;
          fseek(file, pos, SEEK_SET);
          return 1;
      }
      for (int j=0, q; j<cnt; j++) {
        q = read_next_byte (line, i);
        if (q==-1)
          goto file_err;
        i += 2;
        chksum += q;
        *(block->data + blksize) = q;
        blksize++;
      }
      break;
    case 0x01:
    //end of file record
      block->size = blksize;
      return 0;
      break;
    case 0x02:
    //extended address
      off = read_next_word (line, i);
      i += 4;
      chksum += (off>>8) + (off & 0xFF);
      off *= 16;
      block->ext_offset = off;
      break;
    }

    //read the checksum end byte
    i = read_next_byte (line, i);
    if (i==-1)
      goto file_err;
    chksum += i;
    if (chksum & 0xFF) {
      printf ("Checksum error in hex file, line #%d!\n", crtline);
      return -1;
    }

    pos = ftell(file);
  }

file_err:
  printf ("Error in hex file, line #%d\n", crtline);
  return -1;
}
