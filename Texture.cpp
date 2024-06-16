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
	
	for (int i = 0; i < textureData->patchCount; ++i)
		texturePatches.push_back({texturePatch[i].dx, texturePatch[i].dy, texturePatch[i].stepDir, texturePatch[i].colorMap, wad->getPatch(texturePatch[i].pnameIndex)});

	std::vector<int> numPatchesPerColumn(width, 0);
	columnIndices.resize(width, 0);
	columnPatches.resize(width, 0);
	columnYOffsets.resize(width, 0);

	for (int i = 0; i < texturePatches.size(); ++i)
	{
		const Patch *patch =texturePatches[i].patch;
		for (int x = std::max(texturePatches[i].dx, (int16_t)0); x < std::min(width, texturePatches[i].dx + patch->getWidth()); x++)
		{
			numPatchesPerColumn[x]++;
			columnPatches[x] = patch;
			columnYOffsets[x] = texturePatches[i].dy;
			columnIndices[x] = patch->getColumnDataIndex(x - texturePatches[i].dx);
		}
	}

	for (int i = 0; i < width; ++i)	// Cleanup and update
	{
		if (numPatchesPerColumn[i] > 1)	// Is the column covered by more than one patch?
		{
			columnPatches[i] = 0;
			columnIndices[i] = overlap;
			overlap += height;
		}
	}
	
	overlapColumnData = std::unique_ptr<uint8_t[]>(new uint8_t[overlap]);
	for (int i = 0; i < texturePatches.size(); ++i)
	{
		const Patch *patch = texturePatches[i].patch;
		for (int x = std::max(texturePatches[i].dx, (int16_t)0); x < std::min(width, texturePatches[i].dx + patch->getWidth()); x++)
			if (!columnPatches[x]) // Does this column have more than one patch? if yes compose it, else skip it
				patch->composeColumn(overlapColumnData.get() + columnIndices[x], height, patch->getColumnDataIndex(x - texturePatches[i].dx), texturePatches[i].dy);
	}
}

void Texture::render(uint8_t *buf, int rowlen, int screenx, int screeny)
{
	buf += rowlen * screeny + screenx;
    for (int column = 0; column < width; ++column) renderColumn(buf + column, rowlen, column);
}

void Texture::renderColumn(uint8_t *buf, int rowlen, int column)
{
    if (columnPatches[column]) columnPatches[column]->renderColumn(buf, rowlen, columnIndices[column], height, columnYOffsets[column]);
    else for (int y = 0; y < height; y++, buf += rowlen) *buf = overlapColumnData[columnIndices[column] + y];
}
