LIBRARY_OBJ_FILES =        \
	obj/glew.o             \
	obj/AssetBundle.o      \
	obj/BasicPlotter.o     \
	obj/GlobalContext.o    \
	obj/LinePlotter.o      \
	obj/lodepng.o          \
	obj/SampleAsset.o      \
	obj/Shader.o           \
	obj/SimplexNoise.o     \
	obj/SplinePlotter.o    \
	obj/SpriteBatch.o      \
	obj/SpritePlotter.o    \
	obj/TextureAsset.o     \
	obj/TilemapAsset.o     \
	obj/Viewport.o         \
	obj/utils.o         

PLATFORMER_OBJ_FILES =     \
	obj/platformer.o

# COMPILER
CC = clang
CPP = clang++

# NOTE: Depedencies were installed using homebrew.  I had to monkeypatch a few formulas
# to ensure that all libraries were using universal binaries so I can support 32-bit builds
# as well.

# BASE FLAGS
CFLAGS = -Iinclude -Wall -ffast-math
CCFLAGS = -std=c++11  -fno-rtti -fno-exceptions
LIBS = -Llib -framework OpenGL -framework Cocoa -llittlepolygon -lz

# SDL2
#CFLAGS += -D_THREAD_SAFE

LIBS += -framework SDL2 -framework SDL2_mixer

# VECTORIAL
CFLAGS += -Ivectorial/include

# DEBUG FLAGS
CFLAGS += -g -DDEBUG

# OPTIMIZATION FLAGS
# CFLAGS += -Os -flto 

# 32 BITS
CFLAGS += -arch i386
AFLAGS = 32
# AFLAGS = 64

test : bin/platformer bin/platformer.bin
	cp demo/assets/song.mid bin/song.mid
	bin/platformer

bin/platformer: lib/liblittlepolygon.a $(PLATFORMER_OBJ_FILES) 
	mkdir -p bin
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(PLATFORMER_OBJ_FILES)

bin/platformer.bin: demo/assets/* tools/*.py demo/tools/*.py
	mkdir -p bin
	demo/tools/export_game_assets.py demo/assets/assets.yaml $@ $(AFLAGS)

clean:
	rm -f lib/*
	rm -f obj/*
	rm -f bin/*

lib/liblittlepolygon.a: $(LIBRARY_OBJ_FILES)
	mkdir -p lib
	ar rcs $@ $^

obj/%.o: src/%.c
	mkdir -p obj
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.cpp include/*.h
	mkdir -p obj
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: demo/src/%.cpp demo/src/*.h
	mkdir -p obj
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

