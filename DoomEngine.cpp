#include "DoomEngine.h"

DoomEngine::DoomEngine()
: m_WADLoader("DOOM.WAD")
, m_DisplayManager(m_iRenderWidth, m_iRenderHeight, "DIY DOOM")
, m_Player(&m_ViewRenderer, 1)
, m_Map(&m_ViewRenderer, "E1M1", &m_Player, &m_Things)
{
    // Load WAD 
    AssetsManager::GetInstance()->Init(&m_WADLoader);

    // Delay object creation to this point so renderer is inistilized correctly
   
	m_WADLoader.LoadPalette(&m_DisplayManager);
	m_WADLoader.LoadMapData(&m_Map);

    m_ViewRenderer.Init(&m_Map, &m_Player);
    m_Player.Init((m_Map.GetThings())->GetThingByID(m_Player.GetID()));
    m_Map.Init();
}

void DoomEngine::Tick()
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
	uint8_t *pScreenBuffer = m_DisplayManager.GetScreenBuffer();
	m_DisplayManager.InitFrame();
	m_ViewRenderer.Render(pScreenBuffer, m_iRenderWidth);
	m_Player.Render(pScreenBuffer, m_iRenderWidth);
	m_DisplayManager.Render();
	
	// Delay
	SDL_Delay(16); // 1000/60, as int. how many miliseconds per frame
}
