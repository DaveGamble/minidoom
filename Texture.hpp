#pragma once
#include "Patch.hpp"

class Texture
{
public:
    template<typename F> Texture(const uint8_t *ptr, F &&getPatch)
	{
		struct WADTextureData { char textureName[8]; uint32_t alwayszero; uint16_t width, height; uint32_t alwayszero2; uint16_t patchCount; };
		WADTextureData *textureData = (WADTextureData*)ptr;
		struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, alwaysone, alwayszero; };
		WADTexturePatch *texturePatch = (WADTexturePatch*)(ptr + 22);

		width = textureData->width;
		height = textureData->height;
		columns.resize(width);

		for (int i = 0; i < textureData->patchCount; ++i)
		{
			const Patch *patch = getPatch(texturePatch[i].pnameIndex);	// Get the patch
			for (int x = std::max(texturePatch[i].dx, (int16_t)0); x < std::min(width, texturePatch[i].dx + patch->getWidth()); x++)
			{
				if (columns[x].patch)	// This column already has something in
				{
					if (!columns[x].overlap.size())	// Need to render off what's in there!
					{
						columns[x].overlap.resize(height);
						columns[x].patch->composeColumn(columns[x].overlap.data(), height, columns[x].column, columns[x].yOffset);
					}
					patch->composeColumn(columns[x].overlap.data(), height, patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy);	// Render your goodies on top.
				}
				else columns[x] = { patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy, patch, {}};	// Save this as the handler for this column.
			}
		}
	}

	uint16_t pixel(float u, float v) const { int x = u - width * floor(u / width), y = v - height * floor(v / height);
		return (columns[x].overlap.size()) ? columns[x].overlap[y] : columns[x].patch->pixel(columns[x].column, y - columns[x].yOffset);
	}
	void renderColumn(uint8_t *buf, int rowlen, int c, float scale, int yOffset, int yEnd, const uint8_t *lut) const
	{
		if (!columns[c].overlap.size())
			columns[c].patch->renderColumn(buf, rowlen, columns[c].column, yEnd, columns[c].yOffset - yOffset, scale, lut);
		else
			for (int y = yOffset, toy = yOffset * scale; y < height && toy < yEnd + yOffset * scale; y++)
				while (toy < (y + 1) * scale && toy < yEnd + yOffset * scale) buf[(toy++ - (int)(yOffset * scale)) * rowlen] = lut[columns[c].overlap[y]];
	}
	int getWidth() const { return width; }
	int getHeight() const { return height; }
protected:
	int width, height;
	struct TextureColumn { int column, yOffset; const Patch *patch; std::vector<uint8_t> overlap; };
	std::vector<TextureColumn> columns;
};
