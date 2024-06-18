#include "DoomEngine.h"

DoomEngine::DoomEngine(const std::string &wad, const std::string &mapName)
: m_WADLoader(wad)
, m_Map(&m_ViewRenderer, mapName, &m_Things, &m_WADLoader)
, m_ViewRenderer(&m_Map, m_iRenderWidth, m_iRenderHeight)
{
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	m_pWindow = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth, m_iRenderHeight, SDL_WINDOW_SHOWN);
	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
	m_pScreenBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 8, 0, 0, 0, 0);
	m_pRGBBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
	m_pTexture = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	SDL_SetRelativeMouseMode(SDL_TRUE);
	std::vector<uint8_t> palette = m_WADLoader.getLumpNamed("PLAYPAL");
	for (int i = 0; i < 256; ++i) m_ColorPalette[i] = {palette[i * 3 + 0], palette[i * 3 + 1], palette[i * 3 + 2], 255};
	// SDL

	weapon = m_WADLoader.getPatch("PISGA0");
	Thing* t = m_Map.getThings()->GetID(1);
	if (t)
	{
		view.x = t->x;
		view.y = t->y;
		view.angle = t->angle * M_PI / 180;
	}
	view.z = 41;
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
		view.angle += (dt * rotateSpeed); view.angle -= M_PI * 2 * floorf(0.5 * view.angle * M_1_PI);
	};
	
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_MOUSEMOTION:	rotateBy(-0.1*event.motion.xrel);	break;
			case SDL_KEYDOWN:	if (event.key.keysym.sym == SDLK_ESCAPE) m_bIsOver = true;	break;
			case SDL_QUIT:		m_bIsOver = true;	break;
		}
	}
	
	const Uint8* KeyStates = SDL_GetKeyboardState(NULL);
	if (KeyStates[SDL_SCANCODE_W]) { view.x += cos(view.angle) * moveSpeed; view.y += sin(view.angle) * moveSpeed; }
	if (KeyStates[SDL_SCANCODE_A]) { view.x -= sin(view.angle) * moveSpeed; view.y += cos(view.angle) * moveSpeed; }
	if (KeyStates[SDL_SCANCODE_D]) { view.x += sin(view.angle) * moveSpeed; view.y -= cos(view.angle) * moveSpeed; }
	if (KeyStates[SDL_SCANCODE_S]) { view.x -= cos(view.angle) * moveSpeed; view.y -= sin(view.angle) * moveSpeed; }
	if (KeyStates[SDL_SCANCODE_Q]) rotateBy(0.1875f);
	if (KeyStates[SDL_SCANCODE_E]) rotateBy(-0.1875f);
	
	// Update
	view.z = m_Map.getPlayerSubSectorHeight(view) + 41;
	
	// Render
	uint8_t *pScreenBuffer = (uint8_t *)m_pScreenBuffer->pixels;
	SDL_FillRect(m_pScreenBuffer, NULL, 0);
	{
		m_ViewRenderer.render(pScreenBuffer, m_iRenderWidth, view);
// 		weapon->render(pScreenBuffer, m_iRenderWidth, -weapon->getXOffset() * 3, -weapon->getYOffset() * 3, 3);
	}
	SDL_SetPaletteColors(m_pScreenBuffer->format->palette, m_ColorPalette, 0, 256);
	SDL_BlitSurface(m_pScreenBuffer, nullptr, m_pRGBBuffer, nullptr);
	SDL_UpdateTexture(m_pTexture, nullptr, m_pRGBBuffer->pixels, m_pRGBBuffer->pitch);
	SDL_RenderCopy(m_pRenderer, m_pTexture, nullptr, nullptr);
	SDL_RenderPresent(m_pRenderer);
	SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	return m_bIsOver;
}
