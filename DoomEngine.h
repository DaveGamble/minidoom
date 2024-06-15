#pragma once

#include <SDL.h>
#include <string>
#include <memory>

#include "Map.h"
#include "Player.h"
#include "WADLoader.h"
#include "ViewRenderer.h"
#include "AssetsManager.h"

class DoomEngine
{
public:
	DoomEngine();
	~DoomEngine();
	bool Tick();
protected:
	int m_iRenderWidth {320}, m_iRenderHeight {200};
	bool m_bIsOver {false};
	SDL_Surface *m_pScreenBuffer {nullptr}, *m_pRGBBuffer {nullptr};
	SDL_Color m_ColorPallette[256];
	SDL_Window *m_pWindow {nullptr};
	SDL_Renderer *m_pRenderer {nullptr};
	SDL_Texture *m_pTexture {nullptr};

	WADLoader m_WADLoader;
	ViewRenderer m_ViewRenderer;
	Things m_Things;
	Player m_Player;
    Map m_Map;
};
