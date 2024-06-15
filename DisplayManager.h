#pragma once
#include <SDL.h>
#include <string>
#include <vector>

#include "DataTypes.h"

class DisplayManager
{
public:
    DisplayManager(int m_iWindowWidth, int iWindowHeight);
    ~DisplayManager();

    void InitFrame();
    void Render();
    void AddColorPalette(WADPalette &palette);
    
    uint8_t * GetScreenBuffer();
    SDL_Renderer* Init(const std::string &sWindowTitle);
    

protected:
    int m_iScreenWidth;
    int m_iScreenHeight;

    SDL_Color m_ColorPallette[256];

    SDL_Window *m_pWindow;
    SDL_Renderer *m_pRenderer;
    SDL_Texture *m_pTexture;
    SDL_Surface *m_pScreenBuffer;
    SDL_Surface *m_pRGBBuffer;
};

