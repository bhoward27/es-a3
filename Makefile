OUT_DIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)g++

STD = -std=c++17
WFLAGS = -Wall -Werror
OPT = -g -pthread

all: beatbox

beatbox: main.cpp
	$(CC_C) $(WFLAGS) $(OPT) main.cpp udpServer.cpp -o $(OUT_DIR)/beatbox

clean:
	rm -f $(OUT_DIR)/beatbox
	rm -f *.o *.s *.out

