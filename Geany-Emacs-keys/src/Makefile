CC=cc
EXE=emacs_keys.so
CFLAGS= -fPIC `pkg-config --cflags geany`
LDFLAGS=-shared `pkg-config --libs geany`
INSTALL_DIR=$(HOME)/.config/geany/plugins/

all: $(SRC)
	$(CC) $(CFLAGS) -c main.c keymap.c
	$(CC) $(LDFLAGS) -o $(EXE) main.o keymap.o

install:
	cp $(EXE) $(INSTALL_DIR)

tarball:
	tar -zcf ../emacs_keys.tar.gz ../AUTHORS ../COPYING ../README ../src

clean:
#	rm -rf $(EXE)
	rm -rf *.o
