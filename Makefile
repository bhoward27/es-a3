OUT_DIR = $(HOME)/cmpt433/public/myApps

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)g++

STD = -std=c++17
WFLAGS = -Wall -Werror
OPT = -g
LFLAGS = -L$(HOME)/cmpt433/public/asound_lib_BBB
CPP_FILES = main.cpp shutdown_manager.cpp utils.cpp audio_mixer.cpp beat_player.cpp udpServer.cpp

all: beatbox

beatbox: $(CPP_FILES)
	$(CC_C) $(WFLAGS) $(OPT) $(CPP_FILES) -o $(OUT_DIR)/beatbox $(LFLAGS) -lpthread -lasound

clean:
	rm -f $(OUT_DIR)/beatbox
	rm -f *.o *.s *.out

