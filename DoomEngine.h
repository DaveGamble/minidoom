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
	static constexpr float moveSpeed = 8, rotateSpeed = 0.06981317008;
	static constexpr int m_iRenderWidth {960}, m_iRenderHeight {600};
	bool m_bIsOver {false};

	// SDL
	SDL_Surface *screenBuffer {nullptr}, *outputBuffer {nullptr};
	SDL_Color palette[256];
	SDL_Window *window {nullptr};
	SDL_Renderer *sdl_renderer {nullptr};
	SDL_Texture *texture {nullptr};
	// SDL
	
	Viewpoint view;
	WADLoader wad;
	ViewRenderer renderer;
    Map map;
	uint8_t lighting[34][256];
	const Patch *weapon {nullptr};
};
