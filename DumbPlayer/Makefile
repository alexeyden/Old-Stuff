prefix=/usr/local
bindir=$(prefix)/bin
datadir=$(prefix)/share

DC=dmd
EXE=dumbplayer
FLAGS=-L-lm -L-lpthread -L-ldl -L-lgtkd -L-lgstreamerd -L-lphobos2
SRC=main.d player.d

all: $(SRC)
	echo -e "module config;\nconst DATA_DIR = \"$(datadir)/$(EXE)\";" > config.d
	$(DC) -O -release $(SRC) $(FLAGS) -of$(EXE)

clean:
	rm *.o
	rm config.d
#	rm $(EXE)

local:
	echo -e "module config;\nconst DATA_DIR = \"data\";" > config.d
	$(DC) -O -release $(SRC) $(FLAGS) -of$(EXE)
	mkdir ./bin
	mkdir ./bin/data
	cp $(EXE) ./bin/
	cp ./data/* ./bin/data/

install:
	strip $(EXE)
	install -D $(EXE) $(bindir)/$(EXE)
	mkdir -p $(datadir)/$(EXE)
	install -D data/gui.xml $(datadir)/$(EXE)/gui.xml
	install -D data/dumbplayer.desktop $(datadir)/applications/dumbplayer.desktop
	install -D data/scalable.svg $(datadir)/icons/hicolor/scalable/apps/dumbplayer.svg
	install -D data/48.png $(datadir)/icons/hicolor/48x48/apps/dumbplayer.png
	install -D data/24.png $(datadir)/icons/hicolor/24x24/apps/dumbplayer.png

uninstall:
	rm $(bindir)/$(EXE)
	rm $(datadir)/$(EXE)/gui.xml
	rm $(datadir)/applications/dumbplayer.desktop
	rm $(datadir)/icons/hicolor/scalable/apps/dumbplayer.svg
	rm $(datadir)/icons/hicolor/48x48/apps/dumbplayer.png
	rm $(datadir)/icons/hicolor/24x24/apps/dumbplayer.png
	rmdir $(datadir)/$(EXE)
