#include "DoomEngine.h"

DoomEngine::DoomEngine(const char *wadname, const char *mapName)
: renderer(m_iRenderWidth, m_iRenderHeight, wadname, mapName)
{
	if (!renderer.getWad().didLoad()) {printf("The WAD didn't happen. Is it next to the binary? Look in DIYDoom.cpp. In there it asks for [%s] so that's the filename\n", wadname); m_bIsOver = true; return;}
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	window = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_SHOWN);
	sdl_renderer = SDL_CreateRenderer(window, -1, 0);
	screenBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 8, 0, 0, 0, 0);
	outputBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
	texture = SDL_CreateTexture(sdl_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	std::vector<uint8_t> pal = renderer.getWad().getLumpNamed("PLAYPAL");
	for (int i = 0; i < 256; i++) palette[i] = {pal[i * 3 + 0], pal[i * 3 + 1], pal[i * 3 + 2], 255};
	// SDL
	weapon = renderer.getWad().getPatch("PISGA0");
	renderer.getWad().release();
}

DoomEngine::~DoomEngine()
{
	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

bool DoomEngine::Tick()
{
	if (m_bIsOver) return m_bIsOver;
	float mscalar = moveSpeed;
	
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:	renderer.rotateBy(-0.1 * event.motion.xrel * rotateSpeed);	renderer.updatePitch(0.01 * event.motion.yrel); break;
			case SDL_KEYDOWN:	if (event.key.keysym.sym == SDLK_ESCAPE) m_bIsOver = true;	break;
			case SDL_QUIT:		m_bIsOver = true;	break;
		}
	}
	
	const Uint8* keyStates = SDL_GetKeyboardState(NULL);
	if (keyStates[SDL_SCANCODE_LSHIFT] || keyStates[SDL_SCANCODE_RSHIFT]) mscalar *= 3;
	if (keyStates[SDL_SCANCODE_W]) { renderer.moveBy(mscalar, 0); }
	if (keyStates[SDL_SCANCODE_A]) { renderer.moveBy(0, -mscalar); }
	if (keyStates[SDL_SCANCODE_D]) { renderer.moveBy(0, mscalar); }
	if (keyStates[SDL_SCANCODE_S]) { renderer.moveBy(-mscalar, 0); }
	if (keyStates[SDL_SCANCODE_Q]) renderer.rotateBy(0.1875f * rotateSpeed);
	if (keyStates[SDL_SCANCODE_E]) renderer.rotateBy(-0.1875f * rotateSpeed);
	
	// Update
	renderer.updatePlayerSubSectorHeight();
	
	// Render
	uint8_t *pScreenBuffer = (uint8_t *)screenBuffer->pixels;
	SDL_FillRect(screenBuffer, NULL, 0);
	{
		renderer.render(pScreenBuffer, m_iRenderWidth);
//		weapon->render(pScreenBuffer, m_iRenderWidth, -weapon->getXOffset() * 3, -weapon->getYOffset() * 3, lighting[0], 3);
	}
	SDL_SetPaletteColors(screenBuffer->format->palette, palette, 0, 256);
	SDL_BlitSurface(screenBuffer, nullptr, outputBuffer, nullptr);
	SDL_UpdateTexture(texture, nullptr, outputBuffer->pixels, outputBuffer->pitch);
	SDL_RenderCopy(sdl_renderer, texture, nullptr, nullptr);
	SDL_RenderPresent(sdl_renderer);
	SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	return m_bIsOver;
}
