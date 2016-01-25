prefix=/usr/local
bindir=$(prefix)/bin
datadir=$(prefix)/share

CC=gcc
EXE=zenburn_tetris
FLAGS=--std c99 `pkg-config --cflags --libs sdl` -lGL -lGLU
SRC=main.c

all: $(SRC)
	echo "#define DATA_DIR \"$(datadir)/zenburn_tetris\"" > config.h
	$(CC) $(FLAGS) main.c -o $(EXE)	

clean:
	rm config.h
#	rm $(EXE)

local:
	echo "#define DATA_DIR \"./data\"" > config.h
	$(CC) $(FLAGS) main.c -o $(EXE)
	strip zenburn_tetris
	install -D zenburn_tetris ./bin/zenburn_tetris
	install -D data/font.bmp ./bin/data/font.bmp

install:
	strip zenburn_tetris
	install -D zenburn_tetris $(bindir)/zb_tetris
	mkdir -p $(datadir)/zenburn_tetris
	install -D data/font.bmp $(datadir)/zenburn_tetris/font.bmp

uninstall:
	rm $(bindir)/zb_tetris
	rm $(datadir)/zenburn_tetris/font.bmp
	rmdir $(datadir)/zenburn_tetris
