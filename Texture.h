#pragma once
#include "Patch.h"

class Texture
{
public:
    Texture(const uint8_t *ptr, class WADLoader *wad);
	~Texture() {}

    void render(uint8_t *buf, int rowlen, int screenx, int screeny, float scale = 1.0);
    void renderColumn(uint8_t *buf, int rowlen, int firstColumn, float scale = 1.0);
protected:
	int width, height;
	struct TextureColumn { int column, yOffset; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<TextureColumn> columns;
};
