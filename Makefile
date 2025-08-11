flags=-O2 -Wall -std=c2x
.PHONY: all clean
all: clean cvim
cvim: cvim.o
	gcc $^ -o $@ $(flags)
cvim.o: cvim.c cvim.h
	gcc $(flags) -c $<
clean:
	rm -rf *.o cvim
	
