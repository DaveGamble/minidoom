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
	texture.resize(width * height, 0);
	for (int i = 0; i < textureData->patchCount; i++)
	{
		const Patch *patch = wad->getPatch(texturePatch[i].pnameIndex);
		for (int x = std::max(texturePatch[i].dx, (int16_t)0); x < std::min(width, texturePatch[i].dx + patch->getWidth()); x++)
			patch->composeColumn(texture.data() + x * height, height, patch->getColumnDataIndex(x - texturePatch[i].dx), texturePatch[i].dy);
	}
}

void Texture::render(uint8_t *buf, int rowlen, int screenx, int screeny)
{
	buf += rowlen * screeny + screenx;
    for (int column = 0; column < width; ++column) renderColumn(buf + column, rowlen, column);
}

void Texture::renderColumn(uint8_t *buf, int rowlen, int column)
{
    for (int y = 0; y < height; y++, buf += rowlen) *buf = texture[column * height + y];
}
