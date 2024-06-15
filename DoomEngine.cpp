#include "DoomEngine.h"

DoomEngine::DoomEngine() : m_WADLoader("DOOM.WAD")
{
    // Load WAD 
    AssetsManager::GetInstance()->Init(&m_WADLoader);

    m_pDisplayManager = std::unique_ptr < DisplayManager>(new DisplayManager(m_iRenderWidth, m_iRenderHeight));
    m_pDisplayManager->Init("DIY DOOM");

    // Delay object creation to this point so renderer is inistilized correctly
    m_pViewRenderer = std::unique_ptr<ViewRenderer> (new ViewRenderer());
    m_pPlayer = std::unique_ptr<Player>(new Player(m_pViewRenderer.get(), 1));
    m_pMap = std::unique_ptr<Map>(new Map(m_pViewRenderer.get(), "E1M1", m_pPlayer.get(), &m_Things));
   
	m_WADLoader.LoadPalette(m_pDisplayManager.get());
	m_WADLoader.LoadMapData(m_pMap.get());

    m_pViewRenderer->Init(m_pMap.get(), m_pPlayer.get());
    m_pPlayer->Init((m_pMap->GetThings())->GetThingByID(m_pPlayer->GetID()));
    m_pMap->Init();
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
	if (KeyStates[SDL_SCANCODE_UP]) m_pPlayer->MoveForward();
	if (KeyStates[SDL_SCANCODE_DOWN]) m_pPlayer->MoveLeftward();
	if (KeyStates[SDL_SCANCODE_LEFT]) m_pPlayer->RotateLeft();
	if (KeyStates[SDL_SCANCODE_RIGHT]) m_pPlayer->RotateRight();
	if (KeyStates[SDL_SCANCODE_Z]) m_pPlayer->Fly();
	if (KeyStates[SDL_SCANCODE_X]) m_pPlayer->Sink();
	
	// Update
	m_pPlayer->Think(m_pMap->GetPlayerSubSectorHieght());
	
	// Render
	uint8_t *pScreenBuffer = m_pDisplayManager->GetScreenBuffer();
	m_pDisplayManager->InitFrame();
	m_pViewRenderer->Render(pScreenBuffer, m_iRenderWidth);
	m_pPlayer->Render(pScreenBuffer, m_iRenderWidth);
	m_pDisplayManager->Render();
	
	// Delay
	SDL_Delay(16); // 1000/60, as int. how many miliseconds per frame
}
