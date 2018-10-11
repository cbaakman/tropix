CXX = g++
CFLAGS = -std=c++17 -g

FONTFORGE = fontforge


all: bin/tropix bin/resources/tiki.svg


clean:
	rm -f bin/tropix obj/*.o core

MODULES = app error event load game alloc shader texture render/quad

bin/tropix: $(MODULES:%=obj/%.o)
	$(CXX) $(CFLAGS) $^ -lpthread -lboost_filesystem -lboost_system -lSDL2 -lGL -lGLEW -lpng -ltext-gl -lxml-mesh -o $@

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CFLAGS) -c $< -o $@

bin/resources/%.svg: art/%.sfd
	$(FONTFORGE) -lang=ff -c 'Open("$<"); Generate("$@")'
