#pragma once

#include <SDL.h>
#include "Map.hpp"
#include "ViewRenderer.hpp"
#include "WADLoader.hpp"

class DoomEngine
{
public:
	DoomEngine(const std::string &wad, const std::string &mapName);
	~DoomEngine();
	bool Tick();
protected:
	static constexpr float moveSpeed = 4, rotateSpeed = 0.06981317008;
	static constexpr int m_iRenderWidth {960}, m_iRenderHeight {600};
	bool m_bIsOver {false};

	// SDL
	SDL_Surface *m_pScreenBuffer {nullptr}, *m_pRGBBuffer {nullptr};
	SDL_Color m_ColorPalette[256];
	SDL_Window *m_pWindow {nullptr};
	SDL_Renderer *m_pRenderer {nullptr};
	SDL_Texture *m_pTexture {nullptr};
	// SDL
	
	Viewpoint view;
	WADLoader wad;
	ViewRenderer renderer;
    Map map;
	uint8_t lighting[34][256];
	const Patch *weapon {nullptr};
};
