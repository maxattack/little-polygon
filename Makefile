LIBRARY_OBJ_FILES =        \
	obj/glew.o             \
	obj/AssetBundle.o      \
	obj/CirclePlotter.o    \
	obj/CollisionContext.o \
	obj/FkContext.o        \
	obj/LinePlotter.o      \
	obj/SampleAsset.o      \
	obj/SplinePlotter.o    \
	obj/SpriteBatch.o      \
	obj/SpritePlotter.o    \
	obj/TextureAsset.o     \
	obj/TilemapAsset.o     \
	obj/utils.o         

PLATFORMER_OBJ_FILES =     \
	obj/Environment.o      \
	obj/Hero.o             \
	obj/Kitten.o           \
	obj/platformer.o

squids_OBJ_FILES =   \
	obj/SplineTree.o \
    obj/squids.o

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

stest: bin/squids bin/squids.bin
	bin/squids

test : bin/platformer bin/platformer.bin
	cp platformer/assets/song.mid bin/song.mid
	bin/platformer

bin/squids: lib/liblittlepolygon.a $(squids_OBJ_FILES) 
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(squids_OBJ_FILES)

bin/squids.bin: squids/assets/* tools/*.py
	tools/export_asset_bin.py squids/assets/assets.yaml $@ $(AFLAGS)


bin/platformer: lib/liblittlepolygon.a $(PLATFORMER_OBJ_FILES) 
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(PLATFORMER_OBJ_FILES)

bin/platformer.bin: platformer/assets/* common/tools/*.py platformer/tools/*.py
	platformer/tools/export_game_assets.py platformer/assets/assets.yaml $@ $(AFLAGS)

clean:
	rm -f lib/*
	rm -f obj/*
	rm -f bin/*

lib/liblittlepolygon.a: $(LIBRARY_OBJ_FILES)
	ar rcs $@ $^

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o: src/%.cpp include/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: platformer/src/%.cpp platformer/src/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: squids/src/%.cpp # squids/src/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<


