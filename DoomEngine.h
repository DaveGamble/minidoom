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
	bool IsOver() const { return m_bIsOver; }
	void Tick();
protected:
	int m_iRenderWidth {320}, m_iRenderHeight {200};
	bool m_bIsOver {false};
    WADLoader m_WADLoader;
    std::unique_ptr<Map> m_pMap;
    std::unique_ptr<Player> m_pPlayer;
    Things m_Things;
    DisplayManager m_DisplayManager;
    std::unique_ptr<ViewRenderer> m_pViewRenderer;
};
