using LittlePolygon;
using SDL2;
using System;

static class Game {

	static bool gDone = false;

	static void Main(string[] args) {

		FFI.CreateContext("Little Polygon C# Demo", 3 * 320, 3 * 115, "");

		// PLAY SOME MUSIC
		var mus = SDL_mixer.Mix_LoadMUS("song.mid");
		if (mus != IntPtr.Zero) {
			SDL_mixer.Mix_PlayMusic(mus, -1);
		}

		// BASIC EVENT LOOP
		var window = SDL.SDL_GL_GetCurrentWindow();
		while(!gDone) {
			HandleEvents();
			FFI.ClearScreen();
			SDL.SDL_GL_SwapWindow(window);
		}

		FFI.DestroyContext();

	}

	static void HandleEvents() {
		SDL.SDL_Event e;
		while(SDL.SDL_PollEvent(out e) != 0) {
			switch(e.type) {
			case SDL.SDL_EventType.SDL_KEYDOWN:
				HandleKeydown(e.key);
				break;

			case SDL.SDL_EventType.SDL_QUIT:
				gDone = true;
				break;
			default:
				break;
			}
		}
	}

	static void HandleKeydown(SDL.SDL_KeyboardEvent kb) {
		switch(kb.keysym.sym) {
			case SDL.SDL_Keycode.SDLK_ESCAPE:
				gDone = true;
				break;
			default:
				break;
		}
	}
	
}
