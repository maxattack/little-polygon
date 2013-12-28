LIBRARY_OBJ_FILES =     \
	obj/glew.o          \
	obj/AssetBundle.o   \
	obj/Context.o       \
	obj/LinePlotter.o   \
	obj/SampleAsset.o   \
	obj/ShaderAsset.o   \
	obj/SpriteBatch.o   \
	obj/TextureAsset.o  \
	obj/utils.o      

EXAMPLE_OBJ_FILES =     \
	obj/main.o

# COMPILER
CC = clang
CPP = clang++

# BASE FLAGS
# -flto 
CFLAGS = -Iinclude -I/usr/local/include/ -g -Os -Wall 
CCFLAGS = -std=c++11  -fno-rtti
LIBS = -Llib -L/usr/local/lib -framework OpenGL -framework Cocoa -llittlepolygon -lz

# SDL2
#CFLAGS += -D_THREAD_SAFE
CFLAGS += -I/usr/local/include/SDL2
LIBS += -lSDL2 -lSDL2_mixer

# BOX2D
GAME_LIBS = -lBox2D

# CONFIG FLAGS
CFLAGS += -DDEBUG

# 32 BITS
CFLAGS += -arch i386
AFLAGS = 32

test : bin/game bin/assets.bin
	cp example/assets/song.mid bin/song.mid
	bin/game

refresh: bin/game
	tools/export_asset_bin.py example/assets/assets.yaml bin/assets.bin $(AFLAGS)
	bin/game

bin/game: lib/liblittlepolygon.a $(EXAMPLE_OBJ_FILES) 
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(GAME_LIBS) $(EXAMPLE_OBJ_FILES)

bin/assets.bin: example/assets/* tools/*.py
	tools/export_asset_bin.py example/assets/assets.yaml $@ $(AFLAGS)

clean:
	rm -f lib/*
	rm -f obj/*
	rm -f bin/*

lib/liblittlepolygon.a: $(LIBRARY_OBJ_FILES)
	ar rcs $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.cpp
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: example/src/%.cpp
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<
