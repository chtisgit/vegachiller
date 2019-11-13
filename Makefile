
OBJS = vegachiller.o control.o amd.o controls/curve/curve.o

all: vegachiller

build_controls:
	cd controls/curve && $(MAKE)

build_vegachiller: $(OBJS)
	$(CC) $(LFLAGS) -o vegachiller $^ $(LIBS)

vegachiller: build_controls build_vegachiller 
	:

%.o: %.c
	$(CC) $(CFLAGS) -I . -c -o $@ $<

clean:
	$(RM) $(OBJS) vegachiller
	cd controls/curve && $(MAKE) clean

.PHONY: all clean build_controls build_vegachiller