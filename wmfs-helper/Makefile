CC=gcc
DC=dmd
FLAGS=-L-lX11 -L-lasound -L-lm

all: $(SRC)
	$(CC) -c src/c/*.c
	$(DC) $(FLAGS) src/*.d *.o -ofwmfs-helper  

clean:
	rm *.o
