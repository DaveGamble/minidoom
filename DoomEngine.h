#pragma once

#include <SDL.h>
#include "ViewRenderer.hpp"

class DoomEngine
{
public:
	DoomEngine(const char *wad, const char *mapName);
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
	ViewRenderer renderer;
	const Patch *weapon {nullptr};
};
