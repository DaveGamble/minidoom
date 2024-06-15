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
	DoomEngine() {}
	~DoomEngine() {}

    void Render();
	void Quit() { m_bIsOver = true; }
	void Update() { m_pPlayer->Think(m_pMap->GetPlayerSubSectorHieght()); }

	bool IsOver() const { return m_bIsOver; }
    bool Init();

	void ProcessInput();
	void Delay() {SDL_Delay(16);} // 1000/60, as int. how many miliseconds per frame
	
	int GetRenderWidth() const { return m_iRenderWidth; }
	int GetRenderHeight() const { return m_iRenderHeight; }

	std::string GetWADFileName() const { return "DOOM.WAD"; }
	std::string GetAppName() const { return "DIYDOOM"; }

protected:
	int m_iRenderWidth {320}, m_iRenderHeight {200};
	bool m_bIsOver {false};
    WADLoader m_WADLoader;
    std::unique_ptr<Map> m_pMap;
    std::unique_ptr<Player> m_pPlayer;
    std::unique_ptr<Things> m_pThings;
    std::unique_ptr<DisplayManager> m_pDisplayManager;
    std::unique_ptr<ViewRenderer> m_pViewRenderer;
};
