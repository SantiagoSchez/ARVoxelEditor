DIROBJ := obj/
DIREXE := exec/
DIRHEA := include/
DIRSRC := src/

ARTOOLKITDIR := /opt/ARToolKit
INC_DIR := $(ARTOOLKITDIR)/include
LIB_DIR := $(ARTOOLKITDIR)/lib

CFLAGS := -I$(DIRHEA) -I$(INC_DIR) -c -Wall -ggdb
LDFLAGS := -L$(LIB_DIR) -lARgsub -lARvideo -lARMulti -lAR -lglut -lGLU -lGL -lm
CC := gcc

all: dirs arvoxeleditor

dirs:
	mkdir -p $(DIROBJ) $(DIREXE)

arvoxeleditor: $(DIROBJ)functions.o $(DIROBJ)colours.o $(DIROBJ)arvoxeleditor.o
	$(CC) -o $(DIREXE)$@ $^ $(LDFLAGS)

$(DIROBJ)%.o: $(DIRSRC)%.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *~ core $(DIROBJ) $(DIREXE) $(DIRHEA)*~ $(DIRSRC)*~
