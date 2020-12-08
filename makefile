CC=gcc
CFLAGS=-lasound -g -lrt -lpthread -lm
ODIR=build

capture: capture.c
	$(CC) capture.c -o $(ODIR)/capture $(CFLAGS)

playback: playback.c
	$(CC) playback.c -o $(ODIR)/playback $(CFLAGS)

filter: filter.c
	$(CC) filter.c -o $(ODIR)/filter $(CFLAGS)

all: capture playback filter

.PHONY: clean
clean:
	rm -f $(ODIR)/*