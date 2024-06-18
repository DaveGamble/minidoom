#pragma once
#include "Patch.hpp"

class Texture
{
public:
    Texture(const uint8_t *ptr, class WADLoader *wad);
	~Texture() {}

    void render(uint8_t *buf, int rowlen, int screenx, int screeny, float scale = 1.0) const
	{
		buf += rowlen * screeny + screenx;
		for (int column = 0, tox = 0; column < width; ++column) while (tox < (column + 1) * scale) {renderColumn(buf + tox, rowlen, column, scale, 0, height * scale); tox++;}
	}
    void renderColumn(uint8_t *buf, int rowlen, int c, float scale, int yOffset, int yEnd) const
	{
		if (columns[c].overlap.size())
		{
			for (int y = yOffset, toy = yOffset * scale; y < height && toy < yEnd + yOffset * scale; y++) while (toy < (y + 1) * scale && toy < yEnd + yOffset * scale) buf[(toy++ - (int)(yOffset * scale)) * rowlen] = columns[c].overlap[y];
		}
		else columns[c].patch->renderColumn(buf, rowlen, columns[c].column, yEnd, columns[c].yOffset - yOffset, scale);
	}
	int getWidth() const { return width; }
	int getHeight() const { return height; }
protected:
	int width, height;
	struct TextureColumn { int column, yOffset; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<TextureColumn> columns;
};
