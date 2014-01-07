INCLUDES = -I/usr/include/ffmpeg
DEFINES = -D__STDC_CONSTANT_MACROS
LIBS = -lavformat -lavcodec -lswscale -lavutil

ifdef DEBUG
	OPTIM_FLAGS = -O0 -ggdb
else
	OPTIM_FLAGS = -O3
endif

CXXFLAGS = $(OPTIM_FLAGS) $(INCLUDES) $(DEFINES)
LDFLAGS = $(LIBS)

CC = /usr/bin/gcc
C++ = /usr/bin/g++

rawToVideo:	rawToVideo.cpp

rawToVideod:	rawToVideo.cpp
	$(C++) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

debug:
	$(MAKE) DEBUG=1 rawToVideod

clean:
	rm rawToVideo
