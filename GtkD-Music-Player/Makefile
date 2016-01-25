###########################
prefix=/usr/local
bindir=$(prefix)/bin
datadir=$(prefix)/share
portable=false
###########################

DC=dmd
EXE=dumbplayer
FLAGS=-Ilibs/install/include/d/gtkd-2 -L-Llibs/install/lib/
LIBS=-L-ldl -L-lgstreamerd-1 -L-lgtkd-2 -L-lgdk-3
SRC=main.d application.d player.d playlist.d settings.d
UI_FILE=data/main_window.glade
BUILDMODE=-debug

SETTINGS_PLAYLIST_PATH=dumbplayer-playlist.m3u

all: build-libs dumbplayer

dumbplayer: create-config $(SRC:.d=.o)
	$(DC) -O $(BUILDMODE) $(SRC:.d=.o) config.o $(FLAGS) $(LIBS) -of$(EXE)

build-libs:
	@echo "Building GtkD now"
	@cd libs; \
	[ ! -d "install" ] && { \
		mkdir install; \
		make prefix=./install; \
		make prefix=./install gstreamer; \
		make prefix=./install install; \
		make prefix=./install install-gstreamer; \
	} || echo "Already built"; \
	cd ..
	@echo "GtkD building is done"

create-config:
	@echo "Writing config.d..."
	@echo "module config;" > config.d
	@echo "import settings;\n" >> config.d
	@echo "Settings CONFIG_SETTINGS = {" >> config.d
	@echo "\t\"$(SETTINGS_PLAYLIST_PATH)\"," >> config.d
ifeq ($(portable), true)
	@echo "\t\"\"," >> config.d
	@echo -n "\t\`" >> config.d
	@cat $(UI_FILE) >> config.d
	@echo "\`," >> config.d
else
	@echo "\t\"$(datadir)/$(EXE)/ui.xml\"," >> config.d
	@echo "\t\"\"" >> config.d
endif
	@echo "};" >> config.d
	$(DC) -c -O $(BUILDMODE) config.d $(FLAGS)

%.o: %.d
	$(DC) -c -O $(BUILDMODE) $(SRC) $(FLAGS)
	
clean:
	-@rm *.o
	-@rm config.d

clean-libs:
	-cd libs; \
	make clean; \
	rm -Rf install; \
	cd ..
	
install:
	strip $(EXE)
	install -D $(EXE) $(bindir)/$(EXE)
	mkdir -p $(datadir)/$(EXE)
	install -D data/$(UI_FILE) $(datadir)/$(EXE)/ui.xml
	install -D data/dumbplayer.desktop $(datadir)/applications/dumbplayer.desktop
	install -D data/scalable.svg $(datadir)/icons/hicolor/scalable/apps/dumbplayer.svg
	install -D data/48.png $(datadir)/icons/hicolor/48x48/apps/dumbplayer.png
	install -D data/24.png $(datadir)/icons/hicolor/24x24/apps/dumbplayer.png

uninstall:
	rm $(bindir)/$(EXE)
	rm $(datadir)/$(EXE)/ui.xml
	rm $(datadir)/applications/dumbplayer.desktop
	rm $(datadir)/icons/hicolor/scalable/apps/dumbplayer.svg
	rm $(datadir)/icons/hicolor/48x48/apps/dumbplayer.png
	rm $(datadir)/icons/hicolor/24x24/apps/dumbplayer.png
	rmdir $(datadir)/$(EXE)