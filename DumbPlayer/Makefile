prefix=/usr/local
bindir=$(prefix)/bin
datadir=$(prefix)/share

DC=dmd
EXE=dumbplayer
FLAGS=-L-ldl -Ilibs/GtkD/BBB/include/d/gtkd-2
GTKD_LIBS=libs/GtkD/libgstreamerd-1.a libs/GtkD/libgtkd-2.a 
SRC=main.d player.d

BUILDMODE=-debug

FULL_CMD=$(DC) -O $(BUILDMODE) $(FLAGS) $(SRC) config.d $(GTKD_LIBS) -of$(EXE)

all: build-libs dumbplayer

dumbplayer:
	echo -e "module config;\nconst DATA_DIR = \"$(datadir)/$(EXE)\";" > config.d
	$(FULL_CMD)

build-libs:
	echo "BULDING LIBS"

clean:
	rm local/*.o || (rm *.o rm config.d)
#	rm $(EXE)

local: build-libs
	echo -e "module config;\nconst DATA_DIR = \"data\";" > config.d
	$(FULL_CMD:-of$(EXE)=-oflocal/$(EXE))
	test -e "local" || mkdir "local"
	test -e "local/data" || mkdir "local/data"
	cp data/* local/data/

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
