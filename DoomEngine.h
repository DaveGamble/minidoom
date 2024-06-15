#pragma once

#include <SDL.h>
#include <string>
#include <memory>

#include "Map.h"
#include "Player.h"
#include "WADLoader.h"
#include "DisplayManager.h"
#include "ViewRenderer.h"
#include "AssetsManager.h"

class DoomEngine
{
public:
	DoomEngine();
	~DoomEngine() {}
	bool Tick();
protected:
	int m_iRenderWidth {320}, m_iRenderHeight {200};
	bool m_bIsOver {false};
    WADLoader m_WADLoader;
	ViewRenderer m_ViewRenderer;
	Things m_Things;
	DisplayManager m_DisplayManager;
	Player m_Player;
    Map m_Map;
};
