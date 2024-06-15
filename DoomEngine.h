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
	void KeyPressed(SDL_Event &event) { if (event.key.keysym.sym == SDLK_ESCAPE) Quit(); }
    void UpdateKeyStatus(const Uint8* KeyStates);
	void KeyReleased(SDL_Event &event) {}
	void Quit() { m_bIsOver = true; }
	void Update() { m_pPlayer->Think(m_pMap->GetPlayerSubSectorHieght()); }

	bool IsOver() const { return m_bIsOver; }
    bool Init();

	int GetRenderWidth() const { return m_iRenderWidth; }
	int GetRenderHeight() const { return m_iRenderHeight; }
	int GetTimePerFrame() const { return 16; }	// 1000/60, as int. how many miliseconds per frame

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
