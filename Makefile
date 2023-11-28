INCLUDE=raylib/include
LIBS_INCLUDE=raylib/lib/
LIBS=-Wl,-R/home/slamko/proj/cc/autoroute/$(LIBS_INCLUDE) -lraylib -lm

EXE=autoroute
SRC=$(wildcard *.c)
OBJS=$(patsubst %.c,build/%.o,$(SRC))
HEADER=$(wildcard *.h)

all: $(EXE)

$(EXE): $(OBJS)
	gcc $^ -L$(LIBS_INCLUDE) -g $(LIBS) -o $@

build/%.o: %.c $(HEADER)
	mkdir -p build
	gcc -g $< -I$(INCLUDE) -c
	mv $(patsubst %.c,%.o,$<) $@

.PHONY: clean
clean:
	$(RM) -r build
	$(RM) *.o
	$(RM) *.out
	$(RM) $(EXE)
