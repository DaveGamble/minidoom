#include "Texture.h"
#include "WADLoader.hpp"

Texture::Texture(const uint8_t *ptr, WADLoader *wad)
{
	struct WADTextureData { char textureName[8]; uint32_t flags; uint16_t width, height; uint32_t columnDirectory; uint16_t patchCount; };// ColumnDirectory Unused value.
	WADTextureData *textureData = (WADTextureData*)ptr;
	struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, stepDir, colorMap; }; // StepDir, ColorMap Unused values.
	WADTexturePatch *texturePatch = (WADTexturePatch*)(ptr + 22);

	width = textureData->width;
	height = textureData->height;

	columns.resize(width);

	for (int i = 0; i < textureData->patchCount; ++i)
	{
		const Patch *patch = wad->getPatch(texturePatch[i].pnameIndex);	// Get the patch
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
			else
				columns[x] = { patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy, patch, {}};	// Save this as the handler for this column.
		}
	}
}

void Texture::render(uint8_t *buf, int rowlen, int screenx, int screeny)
{
	buf += rowlen * screeny + screenx;
    for (int column = 0; column < width; ++column) renderColumn(buf + column, rowlen, column);
}

void Texture::renderColumn(uint8_t *buf, int rowlen, int c)
{
	if (columns[c].overlap.size()) for (int y = 0; y < height; y++, buf += rowlen) *buf = columns[c].overlap[y];
    else columns[c].patch->renderColumn(buf, rowlen, columns[c].column, height, columns[c].yOffset);
}
