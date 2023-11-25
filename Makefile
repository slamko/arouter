INCLUDE=raylib/include
LIBS_INCLUDE=raylib/lib/
LIBS=-lraylib -lm

all:
	gcc main.c -I$(INCLUDE) -L$(LIBS_INCLUDE) -g -Wl,-R/home/slamko/proj/cc/autoroute/$(LIBS_INCLUDE) $(LIBS) -o autoroute
