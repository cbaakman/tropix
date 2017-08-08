CXX     =   /usr/bin/g++
INCLUDE_DIRS = /usr/include/libxml2
CFLAGS  = -g -D DEBUG $(INCLUDE_DIRS:%=-I%) -std=c++14
CLIENT_LIBS = SDL2 GL GLEW png xml2 cairo unzip boost_filesystem boost_system

all: bin/client

clean:
	rm -f bin/client obj/*.o

obj/%.o: src/%.cpp
	mkdir -p obj
	$(CXX) $(CFLAGS) -c $< -o $@

bin/client: obj/client-main.o obj/client.o obj/settings.o obj/server.o obj/xml.o \
            obj/thread.o obj/event.o obj/play.o obj/texture.o obj/font.o \
            obj/str.o obj/resource.o obj/menu.o obj/exception.o obj/sprite.o \
            obj/fade-screen.o obj/scene.o obj/lang.o obj/provider.o \
            obj/chunk.o obj/render.o obj/shader.o
	mkdir -p bin
	$(CXX) -o $@ $^ $(CLIENT_LIBS:%=-l%)
