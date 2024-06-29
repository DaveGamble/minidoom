#include <SDL.h>
#include "ViewRenderer.hpp"


int main(int argc, char* argv[])
{
	static constexpr int m_iRenderWidth {960}, m_iRenderHeight {600};
//	ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "DOOM2.WAD", "MAP01");
	ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "DOOM.WAD", "E1M1");
//    ViewRenderer engine(m_iRenderWidth, m_iRenderHeight, "freedoom1.WAD", "E1M1");


	if (!engine.didLoadOK()) {printf("The WAD didn't happen. Is it next to the binary? Look at the line before this.\n"); return -1;}
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_SHOWN);
	SDL_Renderer *sdl_renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Surface *screenBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 8, 0, 0, 0, 0);
	SDL_Surface *outputBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
	SDL_Texture *texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	std::vector<uint8_t> pal = engine.getPalette();
	SDL_Color palette[256];
	for (int i = 0; i < 256; i++) palette[i] = {pal[i * 3 + 0], pal[i * 3 + 1], pal[i * 3 + 2], 255};
	// SDL

	static constexpr float moveSpeed = 8, rotateSpeed = 0.06981317008;
	bool isOver = false;


	while (!isOver)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_MOUSEMOTION:	engine.rotateBy(-0.1 * event.motion.xrel * rotateSpeed);	engine.updatePitch(0.01 * event.motion.yrel); break;
				case SDL_KEYDOWN:	if (event.key.keysym.sym == SDLK_ESCAPE) isOver = true;	break;
				case SDL_QUIT:		isOver = true;	break;
			}
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
		
		// Render
		uint8_t *pScreenBuffer = (uint8_t *)screenBuffer->pixels;
		SDL_FillRect(screenBuffer, NULL, 0);
		{
			engine.render(pScreenBuffer, m_iRenderWidth);
		}
		SDL_SetPaletteColors(screenBuffer->format->palette, palette, 0, 256);
		SDL_BlitSurface(screenBuffer, nullptr, outputBuffer, nullptr);
		SDL_UpdateTexture(texture, nullptr, outputBuffer->pixels, outputBuffer->pitch);
		SDL_RenderCopy(sdl_renderer, texture, nullptr, nullptr);
		SDL_RenderPresent(sdl_renderer);
		SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	}

	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
