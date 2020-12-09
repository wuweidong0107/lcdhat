all: lcdhat

LIBS = -lpthread -lfreetype -lm
INCLUDE = -I/usr/include/freetype2/
lcdhat: main.c fb.c input.c
	gcc -g -W -Wall -o $@ $^ $(LIBS) $(INCLUDE)

clean:
	rm lcdhat