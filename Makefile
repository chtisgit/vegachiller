
CFLAGS := -g -O3 -Wall $(CFLAGS)

OBJS = vegachiller.o control.o amd.o controls/curve/curve.o
SDLLIBS = $(shell sdl2-config --libs)
SDLCFLAGS = $(shell sdl2-config --cflags)
IMGUI = vendor/imgui/*.cpp vendor/imgui/examples/imgui_impl_sdl.cpp vendor/imgui/examples/imgui_impl_opengl2.cpp

all: vegachiller vegamaster

build_controls:
	cd controls/curve && $(MAKE)

build_vegachiller: $(OBJS)
	$(CC) $(LFLAGS) -o vegachiller $^ $(LIBS)

vegachiller: build_controls build_vegachiller 
	:

vegamaster: vegamaster.cpp amd.c control.c
	$(CXX) $(SDLCFLAGS) $(CFLAGS) $(CXXFLAGS) -pthread -g -I . -I vendor/imgui -I vendor/imgui/examples -o $@ $^ $(IMGUI) $(LIBS) $(SDLLIBS) -lGL

%.o: %.c
	$(CC) $(CFLAGS) -I . -c -o $@ $<

clean:
	$(RM) $(OBJS) vegachiller
	cd controls/curve && $(MAKE) clean

.PHONY: all clean build_controls build_vegachiller
