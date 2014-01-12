LIBRARY_OBJ_FILES =     \
	obj/glew.o          \
	obj/AssetBundle.o   \
	obj/FkContext.o     \
	obj/GoBinding.o     \
	obj/GoContext.o     \
	obj/LinePlotter.o   \
	obj/SampleAsset.o   \
	obj/SplinePlotter.o \
	obj/SpriteBatch.o   \
	obj/TextureAsset.o  \
	obj/TilemapAsset.o  \
	obj/utils.o         

PLATFORMER_OBJ_FILES =     \
	obj/CollisionSystem.o  \
	obj/Environment.o      \
	obj/Hero.o             \
	obj/Kitten.o           \
	obj/main.o

squids_OBJ_FILES = \
    obj/squids.o

# COMPILER
CC = clang
CPP = clang++

# NOTE: Depedencies were installed using homebrew.  I had to monkeypatch a few formulas
# to ensure that all libraries were using universal binaries so I can support 32-bit builds
# as well.

# BASE FLAGS
CFLAGS = -Icommon/include -I/usr/local/include/ -g -Wall -ffast-math
CCFLAGS = -std=c++11  -fno-rtti -fno-exceptions
LIBS = -Llib -L/usr/local/lib -framework OpenGL -framework Cocoa -llittlepolygon -lz

# SDL2
#CFLAGS += -D_THREAD_SAFE
CFLAGS += -I/usr/local/include/SDL2
LIBS += -lSDL2 -lSDL2_mixer

# VECTORIAL
CFLAGS += -Ivectorial/include

# CONFIG FLAGS
CFLAGS += -DDEBUG

# OPTIMIZATION FLAGS
# CFLAGS += -Os -flto 

# 32 BITS
CFLAGS += -arch i386
AFLAGS = 32

stest: bin/squids
	bin/squids

test : bin/platformer bin/platformer.bin
	cp platformer/assets/song.mid bin/song.mid
	bin/platformer

bin/squids: lib/liblittlepolygon.a $(squids_OBJ_FILES) 
	$(CPP) -o $@ $(CFLAGS) $(CCFLAGS) $(LIBS) $(squids_OBJ_FILES)

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

obj/%.o: common/src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

obj/%.o: common/src/%.cpp common/include/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: platformer/src/%.cpp platformer/src/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<

obj/%.o: squids/src/%.cpp # squids/src/*.h
	$(CPP) $(CFLAGS) $(CCFLAGS) -c -o $@ $<


