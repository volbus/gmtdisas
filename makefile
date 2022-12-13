
CFLAGS= -g -Wall -Werror -o2 -std=gnu99
CC= gcc

all:
	$(CC) $(CFLAGS) gmtdisas.c -o gmtdisas

install:
	cp -u gmtdisas /usr/local/bin/gmtdisas
	-mkdir /usr/share/gmtdisas
	cp -u stm8.inc /usr/share/gmtdisas/

clean:
	rm gmtdisas
