
typedef struct {
  uint32_t start_add;
  uint32_t ext_offset;
  uint32_t size;
  uint32_t line_index;
  unsigned char *data;
} datablock;


int Ihex_Read_Data_Block_Size (FILE *file, datablock *block);
int Ihex_Read_Data_Block (FILE *file, datablock *block);
