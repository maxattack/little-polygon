#include "game.h"

int main(int argc, char *argv[]) {

	lpInitialize("Go, Girl!", 20 * 16 * 4, 8 * 16 * 4, "assets.bin");

	static World world( *lpAssets.userdata<WorldData>("world") );
	world.run();

	lpFinalize();
	return 0;
}



