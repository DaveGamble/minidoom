#pragma once

#include <SDL.h>
#include <string>
#include <memory>

#include "Map.h"
#include "Player.hpp"
#include "WADLoader.hpp"
#include "ViewRenderer.h"

class DoomEngine
{
public:
	DoomEngine(const std::string &wad, const std::string &mapName);
	~DoomEngine();
	bool Tick();
protected:
	int m_iRenderWidth {960}, m_iRenderHeight {600};
	bool m_bIsOver {false};
	// SDL
	SDL_Surface *m_pScreenBuffer {nullptr}, *m_pRGBBuffer {nullptr};
	SDL_Color m_ColorPalette[256];
	SDL_Window *m_pWindow {nullptr};
	SDL_Renderer *m_pRenderer {nullptr};
	SDL_Texture *m_pTexture {nullptr};
	// SDL
	
	WADLoader m_WADLoader;
	ViewRenderer m_ViewRenderer;
	Things m_Things;
	Player m_Player;
    Map m_Map;
};
