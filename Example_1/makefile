PROJECT_ROOT = $(patsubst %/,%,$(dir $(abspath $(lastword $(MAKEFILE_LIST)))))

ifeq ($(OS),Windows_NT)
	# Windows (Cross Compiling in Windows)
	CC      = aarch64-linux-gnu-g++
	OF      = C:/home/pi/openFrameworks
	BRT     = C:/home/pi/brt
	SYSROOT = C:/home/pi/sysroot
	CFLAGS  += --sysroot=$(SYSROOT)
	LFLAGS  += --sysroot=$(SYSROOT)
	LFLAGS  += -L$(SYSROOT)/lib
	LFLAGS  += -L$(SYSROOT)/lib/aarch64-linux-gnu
	LFLAGS  += -L$(SYSROOT)/usr/lib
	LFLAGS  += -L$(SYSROOT)/usr/lib/aarch64-linux-gnu
	LFLAGS  += -Wl,-rpath-link,$(SYSROOT)/lib/aarch64-linux-gnu
	LFLAGS  += -Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-gnu
else
	# Linux (Native Compiling in Raspberry Pi)
	CC      = g++
	OF      = /home/pi/openFrameworks
	BRT     = /home/pi/brt
endif

INC_DIR += -I$(OF)/libs/glm/include
INC_DIR += -I$(OF)/libs/json/include
INC_DIR += -I$(OF)/libs/kiss/include
INC_DIR += -I$(OF)/libs/tess2/include
INC_DIR += -I$(OF)/libs/utf8/include
INC_DIR += -I$(OF)/libs/openFrameworks
INC_DIR += -I$(OF)/libs/openFrameworks/3d
INC_DIR += -I$(OF)/libs/openFrameworks/app
INC_DIR += -I$(OF)/libs/openFrameworks/communication
INC_DIR += -I$(OF)/libs/openFrameworks/events
INC_DIR += -I$(OF)/libs/openFrameworks/gl
INC_DIR += -I$(OF)/libs/openFrameworks/graphics
INC_DIR += -I$(OF)/libs/openFrameworks/math
INC_DIR += -I$(OF)/libs/openFrameworks/sound
INC_DIR += -I$(OF)/libs/openFrameworks/types
INC_DIR += -I$(OF)/libs/openFrameworks/utils
INC_DIR += -I$(OF)/libs/openFrameworks/video
INC_DIR += -I/usr/include/cairo

INC_DIR += -I$(BRT)/include
INC_DIR += -I$(BRT)/include/third_party_libraries/eigen

LIB_A += $(OF)/libs/openFrameworksCompiled/lib/linuxaarch64/libopenFrameworks.a
LIB_A += $(OF)/libs/kiss/lib/linuxaarch64/libkiss.a
LIB_A += $(OF)/libs/tess2/lib/linuxaarch64/libtess2.a
LIB_A += $(BRT)/include/third_party_libraries/libmysofa/lib/aarch64/libmysofa.a

CFLAGS += $(INC_DIR) -std=c++17 -g

LFLAGS += -lgstapp-1.0 -lgstvideo-1.0 -lgstbase-1.0 -lgstreamer-1.0 -ludev -lfontconfig -lfreetype -lsndfile -lopenal -lcurl -lglfw -lrtaudio -lpulse-simple -lpulse -pthread -lasound -lGLEW -lGL -lX11 -lGLU -lz -lharfbuzz -lcairo-gobject -lcairo -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lmpg123 -lGLESv1_CM -lGLESv2 -lEGL -lglut -lXrandr -lXxf86vm -lXi -lXcursor -ldl -lpthread -lfreeimage -lboost_filesystem -lboost_system -lpugixml -luriparser -lXinerama -Wl,--as-needed -Wl,--gc-sections -lstdc++fs -no-pie

all: rbp

rbp: $(PROJECT_ROOT)/build/main.o $(PROJECT_ROOT)/build/ofApp.o
	$(CC) -o $(PROJECT_ROOT)/bin/rbp $(PROJECT_ROOT)/build/main.o $(PROJECT_ROOT)/build/ofApp.o $(LIB_A) $(LFLAGS)

$(PROJECT_ROOT)/build/main.o: $(PROJECT_ROOT)/src/main.cpp
	$(CC) -c $(PROJECT_ROOT)/src/main.cpp -o $(PROJECT_ROOT)/build/main.o $(CFLAGS)

$(PROJECT_ROOT)/build/ofApp.o: $(PROJECT_ROOT)/src/ofApp.h $(PROJECT_ROOT)/src/ofApp.cpp
	$(CC) -c $(PROJECT_ROOT)/src/ofApp.cpp -o $(PROJECT_ROOT)/build/ofApp.o $(CFLAGS)

clean:
	rm -f $(PROJECT_ROOT)/build/main.o
	rm -f $(PROJECT_ROOT)/build/ofApp.o
	rm -f $(PROJECT_ROOT)/bin/rbp