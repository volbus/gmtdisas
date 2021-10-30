# gmtdisas
STM8 disassembler

This is a disassembler for STM8.

It will take as input an intel hex file, with the code, and decode it into assembly language.
The assembly language is compatible with the asst8 assembler, see Alan R. Baldwin's ASxxxx assemblers and relocating linker.

To install just run install.sh as sudo, after you make sure the execution mode bit is set.

Usage: gmtdisas [option] [<ihex_file>]
For usage details call the program with the -h (or --help) option to print help:
  `gmtdisas -h`
