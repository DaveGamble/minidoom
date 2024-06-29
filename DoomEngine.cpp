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
	
	const Thing* t = renderer.getThing(1);
	if (t)
	{
		view.x = t->x;
		view.y = t->y;
		view.angle = t->angle * M_PI / 180;
		view.cosa = cos(view.angle);
		view.sina = sin(view.angle);
	}
	view.z = 41;
	view.pitch = 0;
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
	auto rotateBy = [&](float dt) {
		view.angle += (dt * rotateSpeed);
		view.angle -= M_PI * 2 * floorf(0.5 * view.angle * M_1_PI);
		view.cosa = cos(view.angle);
		view.sina = sin(view.angle);
	};
	auto moveBy = [&](float dx, float dy) {
		if (renderer.doesLineIntersect(view.x, view.y, view.x + dx * 4, view.y + dy * 4)) return;
		view.x += dx;
		view.y += dy;
	};
	
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:	rotateBy(-0.1*event.motion.xrel);	view.pitch = std::clamp(view.pitch - 0.01 * event.motion.yrel, -1.0, 1.0); break;
			case SDL_KEYDOWN:	if (event.key.keysym.sym == SDLK_ESCAPE) m_bIsOver = true;	break;
			case SDL_QUIT:		m_bIsOver = true;	break;
		}
	}
	
	const Uint8* keyStates = SDL_GetKeyboardState(NULL);
	if (keyStates[SDL_SCANCODE_LSHIFT] || keyStates[SDL_SCANCODE_RSHIFT]) mscalar *= 3;
	if (keyStates[SDL_SCANCODE_W]) { moveBy(view.cosa * mscalar, view.sina * mscalar); }
	if (keyStates[SDL_SCANCODE_A]) { moveBy(-view.sina * mscalar, view.cosa * mscalar); }
	if (keyStates[SDL_SCANCODE_D]) { moveBy(view.sina * mscalar, -view.cosa * mscalar); }
	if (keyStates[SDL_SCANCODE_S]) { moveBy(-view.cosa * mscalar, -view.sina * mscalar); }
	if (keyStates[SDL_SCANCODE_Q]) rotateBy(0.1875f);
	if (keyStates[SDL_SCANCODE_E]) rotateBy(-0.1875f);
	
	// Update
	view.z = renderer.getPlayerSubSectorHeight(view) + 41;
	
	// Render
	uint8_t *pScreenBuffer = (uint8_t *)screenBuffer->pixels;
	SDL_FillRect(screenBuffer, NULL, 0);
	{
		renderer.render(pScreenBuffer, m_iRenderWidth, view);
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
