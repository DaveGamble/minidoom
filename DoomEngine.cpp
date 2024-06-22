#include "DoomEngine.h"

DoomEngine::DoomEngine(const std::string &wadname, const std::string &mapName)
: wad(wadname)
, map(mapName, wad)
, renderer(m_iRenderWidth, m_iRenderHeight)
{
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	m_pWindow = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_SHOWN);
	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
	m_pScreenBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 8, 0, 0, 0, 0);
	m_pRGBBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
	m_pTexture = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	std::vector<uint8_t> palette = wad.getLumpNamed("PLAYPAL");
	for (int i = 0; i < 256; ++i) m_ColorPalette[i] = {palette[i * 3 + 0], palette[i * 3 + 1], palette[i * 3 + 2], 255};
	// SDL

	weapon = wad.getPatch("PISGA0");
	const Thing* t = map.getThing(1);
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
	SDL_DestroyRenderer(m_pRenderer);
	SDL_DestroyWindow(m_pWindow);
	SDL_Quit();
}

bool DoomEngine::Tick()
{
	auto rotateBy = [&](float dt) {
		view.angle += (dt * rotateSpeed);
		view.angle -= M_PI * 2 * floorf(0.5 * view.angle * M_1_PI);
		view.cosa = cos(view.angle);
		view.sina = sin(view.angle);
	};
	auto moveBy = [&](float dx, float dy) {
		if (map.doesLineIntersect(view.x, view.y, view.x + dx * 4, view.y + dy * 4)) return;
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
	
	const Uint8* KeyStates = SDL_GetKeyboardState(NULL);
	if (KeyStates[SDL_SCANCODE_W]) { moveBy(view.cosa * moveSpeed, view.sina * moveSpeed); }
	if (KeyStates[SDL_SCANCODE_A]) { moveBy(-view.sina * moveSpeed, view.cosa * moveSpeed); }
	if (KeyStates[SDL_SCANCODE_D]) { moveBy(view.sina * moveSpeed, -view.cosa * moveSpeed); }
	if (KeyStates[SDL_SCANCODE_S]) { moveBy(-view.cosa * moveSpeed, -view.sina * moveSpeed); }
	if (KeyStates[SDL_SCANCODE_Q]) rotateBy(0.1875f);
	if (KeyStates[SDL_SCANCODE_E]) rotateBy(-0.1875f);
	
	// Update
	view.z = map.getPlayerSubSectorHeight(view) + 41;
	
	// Render
	uint8_t *pScreenBuffer = (uint8_t *)m_pScreenBuffer->pixels;
	SDL_FillRect(m_pScreenBuffer, NULL, 0);
	{
		renderer.render(pScreenBuffer, m_iRenderWidth, view, map);
		uint8_t lut[256];for (int i = 0; i < 256; i++) lut[i] = i;
		weapon->render(pScreenBuffer, m_iRenderWidth, -weapon->getXOffset() * 3, -weapon->getYOffset() * 3, lut, 3);
	}
	SDL_SetPaletteColors(m_pScreenBuffer->format->palette, m_ColorPalette, 0, 256);
	SDL_BlitSurface(m_pScreenBuffer, nullptr, m_pRGBBuffer, nullptr);
	SDL_UpdateTexture(m_pTexture, nullptr, m_pRGBBuffer->pixels, m_pRGBBuffer->pitch);
	SDL_RenderCopy(m_pRenderer, m_pTexture, nullptr, nullptr);
	SDL_RenderPresent(m_pRenderer);
	SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	return m_bIsOver;
}
