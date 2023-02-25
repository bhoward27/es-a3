# OUT_DIR = $(HOME)/cmpt433/public/myApps

# CROSS_COMPILE = arm-linux-gnueabihf-
# CC_C = $(CROSS_COMPILE)g++

# STD = -std=c++17
# WFLAGS = -Wall -Werror
# OPT = -g

# all: beatbox

# beatbox: main.cpp
# 	$(CC_C) $(WFLAGS) $(OPT) main.cpp -o $(OUT_DIR)/beatbox

# clean:
# 	rm -f $(OUT_DIR)/beatbox
# 	rm -f *.o *.s *.out

# makefile code below provided by Dr. Brian Fraser
# Learned how to compile multiple c programs in a makefile from this link: https://stackoverflow.com/questions/5950395/makefile-to-compile-multiple-c-programs
OUTFILE_BEATBOX = beatbox

OUTDIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)gcc
CFLAGS_BEATBOX = -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -Wshadow -pthread

all: beatbox
beatbox: 
	$(CC_C) $(CFLAGS_BEATBOX) main.c udpServer.c -o $(OUTDIR)/$(OUTFILE_BEATBOX)
clean:
	rm $(OUTDIR)/$(OUTFILE_BEATBOX)
