CXX = g++
CFLAGS = -std=c++17 -g

FONTFORGE = fontforge
INKSCAPE = inkscape


all: bin/tropix bin/resources/tiki.svg bin/resources/textures/sand.png bin/resources/textures/horizon.png


clean:
	rm -rf bin/tropix obj/* core

MODULES = app error event load game alloc shader texture ground water sky noise chunk text

bin/tropix: $(MODULES:%=obj/%.o)
	mkdir -p $(@D)
	$(CXX) $(CFLAGS) $^ -lpthread -lboost_filesystem -lboost_system -lSDL2 -lGL -lGLEW -lpng -ltext-gl -lxml-mesh -o $@

obj/%.o: src/%.cpp
	mkdir -p $(@D)
	$(CXX) $(CFLAGS) -c $< -o $@

bin/resources/%.svg: art/%.sfd
	mkdir -p $(@D)
	$(FONTFORGE) -lang=ff -c 'Open("$<"); Generate("$@")'

bin/resources/textures/%.png: art/%.svg
	mkdir -p $(@D)
	$(INKSCAPE) --file=$< --export-png=$@
