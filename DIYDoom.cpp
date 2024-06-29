#include <SDL.h>
#include "ViewRenderer.hpp"

int main(int argc, char* argv[])
{
	static constexpr float moveSpeed = 8, rotateSpeed = 0.06981317008;
	static constexpr int m_iRenderWidth {960}, m_iRenderHeight {600};
//	ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "DOOM2.WAD", "MAP01");
	ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "DOOM.WAD", "E1M1");
//    ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "freedoom1.WAD", "E1M1");


	if (!engine.didLoadOK()) {printf("The WAD didn't happen. Is it next to the binary? Look at the line before this.\n"); return -1;}
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_SHOWN);
	SDL_Renderer *sdl_renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Texture *texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	// SDL

	bool isOver = false;
	while (!isOver)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_MOUSEMOTION) { engine.rotateBy(-0.1 * event.motion.xrel * rotateSpeed);	engine.updatePitch(0.01 * event.motion.yrel); }
			else if (event.type == SDL_QUIT || (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) isOver = true;
		}
		
		float mscalar = moveSpeed;
		const Uint8* keyStates = SDL_GetKeyboardState(NULL);
		if (keyStates[SDL_SCANCODE_LSHIFT] || keyStates[SDL_SCANCODE_RSHIFT]) mscalar *= 3;
		if (keyStates[SDL_SCANCODE_W]) engine.moveBy(mscalar, 0);
		if (keyStates[SDL_SCANCODE_A]) engine.moveBy(0, -mscalar);
		if (keyStates[SDL_SCANCODE_D]) engine.moveBy(0, mscalar);
		if (keyStates[SDL_SCANCODE_S]) engine.moveBy(-mscalar, 0);
		if (keyStates[SDL_SCANCODE_Q]) engine.rotateBy(0.1875f * rotateSpeed);
		if (keyStates[SDL_SCANCODE_E]) engine.rotateBy(-0.1875f * rotateSpeed);
		
		void *pixels; int pitch;
		if (!SDL_LockTexture(texture, NULL, &pixels, &pitch))
		{
			engine.render((uint8_t *)pixels, pitch);	// Render
			SDL_UnlockTexture(texture);
		}
		SDL_RenderCopy(sdl_renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(sdl_renderer);
		SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	}
	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
