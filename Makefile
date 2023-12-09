INCLUDE=-Iraylib/include -Iraygui-4.0/src/
LIBS_INCLUDE=raylib/lib/
LIBS=-Wl,-R/home/slamko/proj/cc/autoroute/$(LIBS_INCLUDE) -lraylib -lm

EXE=autoroute
SRC=$(wildcard *.c)
CPP_SRC=$(wildcard *.cpp)
OBJS=$(patsubst %.c,build/%.o,$(SRC))
OBJS+=$(patsubst %.cpp,build/%.o,$(CPP_SRC))
HEADER=$(wildcard *.h)

all: $(EXE)

$(EXE): $(OBJS)
	g++ $^ -L$(LIBS_INCLUDE) -g $(LIBS) -o $@

build/%.o: %.cpp $(HEADER)
	mkdir -p build
	g++ -ggdb $< $(INCLUDE) -c
	mv $(patsubst %.cpp,%.o,$<) $@

build/%.o: %.c $(HEADER)
	mkdir -p build
	gcc -ggdb $< $(INCLUDE) -c
	mv $(patsubst %.c,%.o,$<) $@

.PHONY: clean
clean:
	$(RM) -r build
	$(RM) *.o
	$(RM) *.out
	$(RM) $(EXE)
