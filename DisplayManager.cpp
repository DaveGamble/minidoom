#include "DisplayManager.h"
#include <iostream>

DisplayManager::DisplayManager(int iWindowWidth, int iWindowHeight, const std::string &sWindowTitle) : m_iScreenWidth(iWindowWidth), m_iScreenHeight(iWindowHeight), m_pWindow(nullptr), m_pRenderer(nullptr)
{
	int bpp;
	uint32_t Rmask;
	uint32_t Gmask;
	uint32_t Bmask;
	uint32_t Amask;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		std::cout << "SDL failed to initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	m_pWindow = SDL_CreateWindow(sWindowTitle.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_iScreenWidth * 3, m_iScreenHeight * 3, SDL_WINDOW_SHOWN);
	if (m_pWindow == nullptr)
	{
		std::cout << "SDL failed to create window! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	m_pRenderer = SDL_CreateRenderer(m_pWindow, -1, 0);
	if (m_pRenderer == nullptr)
	{
		std::cout << "SDL failed to create renderer! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	uint32_t PixelFormat = SDL_GetWindowPixelFormat(m_pWindow);

	// Create 8-bit screen buffer
	m_pScreenBuffer = SDL_CreateRGBSurface(0, m_iScreenWidth, m_iScreenHeight, 8, 0, 0, 0, 0);
	if (m_pScreenBuffer == nullptr)
	{
		std::cout << "SDL failed to create 8-bit surface! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	SDL_FillRect(m_pScreenBuffer, NULL, 0);

	SDL_PixelFormatEnumToMasks(PixelFormat, &bpp, &Rmask, &Gmask, &Bmask, &Amask);

	m_pRGBBuffer = SDL_CreateRGBSurface(0, m_iScreenWidth, m_iScreenHeight, 32, Rmask, Gmask, Bmask, Amask);
	if (m_pScreenBuffer == nullptr)
	{
		std::cout << "SDL failed to create RGB surface! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}

	SDL_FillRect(m_pRGBBuffer, NULL, 0);

	m_pTexture = SDL_CreateTexture(m_pRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_iScreenWidth, m_iScreenHeight);
	if (m_pTexture == nullptr)
	{
		std::cout << "SDL failed to create texture! SDL_Error: " << SDL_GetError() << std::endl;
		return;
	}
}


DisplayManager::~DisplayManager()
{
    SDL_DestroyRenderer(m_pRenderer);
    SDL_DestroyWindow(m_pWindow);
    SDL_Quit();
}

void DisplayManager::AddColorPalette(WADPalette &palette)
{
	for (int i = 0; i < 256; ++i)
	{
		m_ColorPallette[i].r = palette.bytes[i * 3 + 0];
		m_ColorPallette[i].g = palette.bytes[i * 3 + 1];
		m_ColorPallette[i].b = palette.bytes[i * 3 + 2];
		m_ColorPallette[i].a = 255;
	}
}

void DisplayManager::InitFrame()
{
    SDL_FillRect(m_pScreenBuffer, NULL, 0);
}

void DisplayManager::Render()
{
    SDL_SetPaletteColors(m_pScreenBuffer->format->palette, m_ColorPallette, 0, 256);

    SDL_BlitSurface(m_pScreenBuffer, nullptr, m_pRGBBuffer, nullptr);
    SDL_UpdateTexture(m_pTexture, nullptr, m_pRGBBuffer->pixels, m_pRGBBuffer->pitch);
    SDL_RenderCopy(m_pRenderer, m_pTexture, nullptr, nullptr);
    SDL_RenderPresent(m_pRenderer);
}

uint8_t *DisplayManager::GetScreenBuffer()
{
    return (uint8_t *)m_pScreenBuffer->pixels;
}
