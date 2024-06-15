#include "DoomEngine.h"

DoomEngine::DoomEngine()
: m_WADLoader("DOOM.WAD")
, assets(&m_WADLoader)
, m_Player(&m_ViewRenderer, 1)
, m_Map(&m_ViewRenderer, "E1M1", &m_Player, &m_Things)
{
	// SDL
	SDL_Init(SDL_INIT_EVERYTHING);
	m_pWindow = SDL_CreateWindow("DIY DOOM", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iRenderWidth * 3, m_iRenderHeight * 3, SDL_WINDOW_SHOWN);
	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
	m_pScreenBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 8, 0, 0, 0, 0);
	m_pRGBBuffer = SDL_CreateRGBSurface(0, m_iRenderWidth, m_iRenderHeight, 32, 0xff0000, 0xff00, 0xff, 0xff000000);
	m_pTexture = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iRenderWidth, m_iRenderHeight);
	const uint8_t *palette = m_WADLoader.GetPalette();
	for (int i = 0; i < 256; ++i) m_ColorPalette[i] = {palette[i * 3 + 0], palette[i * 3 + 1], palette[i * 3 + 2], 255};
	// SDL

	m_WADLoader.LoadMapData(&m_Map);
    m_ViewRenderer.Init(&m_Map, &m_Player);
    m_Player.Init((m_Map.GetThings())->GetThingByID(m_Player.GetID()), &assets);
    m_Map.Init();
}

DoomEngine::~DoomEngine()
{
	SDL_DestroyRenderer(m_pRenderer);
	SDL_DestroyWindow(m_pWindow);
	SDL_Quit();
}

bool DoomEngine::Tick()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_KEYDOWN:	if (event.key.keysym.sym == SDLK_ESCAPE) m_bIsOver = true;	break;
			case SDL_QUIT:		m_bIsOver = true;	break;
		}
	}
	
	const Uint8* KeyStates = SDL_GetKeyboardState(NULL);
	if (KeyStates[SDL_SCANCODE_UP]) m_Player.MoveForward();
	if (KeyStates[SDL_SCANCODE_DOWN]) m_Player.MoveLeftward();
	if (KeyStates[SDL_SCANCODE_LEFT]) m_Player.RotateLeft();
	if (KeyStates[SDL_SCANCODE_RIGHT]) m_Player.RotateRight();
	if (KeyStates[SDL_SCANCODE_Z]) m_Player.Fly();
	if (KeyStates[SDL_SCANCODE_X]) m_Player.Sink();
	
	// Update
	m_Player.Think(m_Map.GetPlayerSubSectorHieght());
	
	// Render
	uint8_t *pScreenBuffer = (uint8_t *)m_pScreenBuffer->pixels;
	SDL_FillRect(m_pScreenBuffer, NULL, 0);
	{
		m_ViewRenderer.Render(pScreenBuffer, m_iRenderWidth);
		m_Player.Render(pScreenBuffer, m_iRenderWidth);
	}
	SDL_SetPaletteColors(m_pScreenBuffer->format->palette, m_ColorPalette, 0, 256);
	SDL_BlitSurface(m_pScreenBuffer, nullptr, m_pRGBBuffer, nullptr);
	SDL_UpdateTexture(m_pTexture, nullptr, m_pRGBBuffer->pixels, m_pRGBBuffer->pitch);
	SDL_RenderCopy(m_pRenderer, m_pTexture, nullptr, nullptr);
	SDL_RenderPresent(m_pRenderer);
	SDL_Delay(16); // wait 1000/60, as int. how many miliseconds per frame
	return m_bIsOver;
}
