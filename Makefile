all: lcdhat

LIBS = -lpthread -lfreetype -lm
INCLUDE = -I/usr/include/freetype2/
lcdhat: main.c fb.c input.c
	gcc -g -W -Wall -o $@ $^ $(LIBS) $(INCLUDE)

install:
	install -m 0755 lcdhat-helper.sh /usr/local/bin/
	install -m 0755 lcdhat /usr/local/bin/
clean:
	rm -f lcdhat