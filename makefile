CC=gcc
CFLAGS=-lasound
ODIR=build

capture: capture.c
	$(CC) -c capture.c -o $(ODIR)/capture $(CFLAGS)

playback: playback.c
	$(CC) -c playback.c -o $(ODIR)/playback $(CFLAGS)

all: capture playback

.PHONY: clean
clean:
	rm -f $(ODIR)/*