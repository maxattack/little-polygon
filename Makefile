LIBRARY_OBJ_FILES =     \
	obj/glew.o          \
	obj/AssetBundle.o   \
	obj/LinePlotter.o   \
	obj/SampleAsset.o   \
	obj/SpriteBatch.o   \
	obj/TextureAsset.o  \
	obj/TilemapAsset.o  \
	obj/utils.o         

game_OBJ_FILES =           \
	obj/CollisionSystem.o  \
	obj/Environment.o      \
	obj/Hero.o             \
	obj/Kitten.o           \
	obj/main.o

# COMPILER
CC = clang
CPP = clang++

# NOTE: Depedencies were installed using homebrew.  I had to monkeypatch a few formulas
# to ensure that all libraries were using universal binaries so I can support 32-bit builds
# as well.

# BASE FLAGS
CFLAGS = -Icommon/include -I/usr/local/include/ -g -Os -Wall -flto -ffast-math
CCFLAGS = -std=c++11  -fno-rtti -fno-exceptions
LIBS = -Llib -L/usr/local/lib -framework OpenGL -framework Cocoa -llittlepolygon -lz

# SDL2
#CFLAGS += -D_THREAD_SAFE
CFLAGS += -I/usr/local/include/SDL2
LIBS += -lSDL2 -lSDL2_mixer

# BOX2D
# GAME_LIBS = -lBox2D

# CONFIG FLAGS
CFLAGS += -DDEBUG

# 32 BITS
CFLAGS += -arch i386
AFLAGS = 32

test : bin/game bin/assets.bin
	cp game/assets/song.mid bin/song.mid
	bin/game

bin/game: lib/liblittlepolygon.a $(game_OBJ_FILES) 
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(GAME_LIBS) $(game_OBJ_FILES)

bin/assets.bin: game/assets/* common/tools/*.py game/tools/*.py
	game/tools/export_game_assets.py game/assets/assets.yaml $@ $(AFLAGS)

clean:
	rm -f lib/*
	rm -f obj/*
	rm -f bin/*

lib/liblittlepolygon.a: $(LIBRARY_OBJ_FILES)
	ar rcs $@ $^

obj/%.o: common/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o: common/src/%.cpp common/include/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: game/src/%.cpp game/src/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<
