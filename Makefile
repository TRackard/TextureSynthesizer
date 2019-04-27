INC_DIR = ./header
SCR_DIR = ./source/
CFLAGS = -Wall -I$(INC_DIR) -std=c99
SOURCES = $(SCR_DIR)*.c
OUT = synth
LIBS = -lm

default:
	gcc $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
debug:
	gcc -g $(CFLAGS) $(SOURCES) $(LIBS) -o $(OUT)
clean:
	rm -f $(OUT)* out* #out is the name of the picture output

