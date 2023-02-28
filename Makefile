PUBDIR = $(HOME)/cmpt433/public/myApps
OUTDIR = $(PUBDIR)
WAV_PATH = $(PUBDIR)/beatbox-wav-files/
NODE_COPY_PATH = $(PUBDIR)/beatbox-server-copy
TARGET = beatbox

CROSS_COMPILE = arm-linux-gnueabihf-
CC_C = $(CROSS_COMPILE)g++

STD = -std=c++17
WFLAGS = -Wall -Werror
OPT = -g
CFLAGS = $(WFLAGS) $(OPT) -D _POSIX_C_SOURCE=200809L
LFLAGS = -L$(HOME)/cmpt433/public/asound_lib_BBB
SOURCES = main.cpp shutdown_manager.cpp utils.cpp audio_mixer.cpp beat_player.cpp udpServer.cpp joystick.cpp

all: cpp_app node wav

cpp_app: $(SOURCES)
	@echo 'COMPILING C++ CODE'
	@echo ''
	$(CC_C) $(CFLAGS) $(SOURCES) -o $(OUTDIR)/$(TARGET) $(LFLAGS) -lpthread -lasound

wav:
	@echo 'COPYING WAVE FILES TO $(WAV_PATH)'
	@echo ''
	mkdir -p $(WAV_PATH)

# spinner.sh displays a loading animation to show the command (in this case, cp) is still running.
# cp could take a long time, so want feedback to confirm that the VM isn't frozen!
	./spinner.sh cp -R beatbox-wav-files/* $(WAV_PATH)

node: node_deploy node_install

node_deploy:
	@echo 'COPYING THE NODE.JS FILES TO $(NODE_COPY_PATH)'
	@echo ''
	mkdir -p $(NODE_COPY_PATH)
	chmod a+rwx $(NODE_COPY_PATH)

	./spinner.sh cp -rf NodeJs-Code/* $(NODE_COPY_PATH)

	@echo 'Do not edit any files in this folder; they are copied!' > $(NODE_COPY_PATH)/DO_NOT_EDIT-FILES_COPIED.txt
	@echo ''
	@echo 'NOTE: On the host, in $(NODE_COPY_PATH), it is best to run: npm install'
	@echo '      Or, just run the node_install target in this makefile.'

node_install:
	@echo ''
	@echo ''
	@echo 'INSTALLING REQUIRED NODE PACKAGES'
	@echo '(This may take some time)'
	@echo ''
	cd $(NODE_COPY_PATH) && npm install

clean:
	rm -f $(OUTDIR)/$(TARGET)
	rm -f *.o *.s *.out

