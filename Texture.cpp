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

	numPatchesPerColumn.resize(width, 0);
	columnIndices.resize(width, 0);
	columnPatches.resize(width, 0);

	for (int i = 0; i < texturePatches.size(); ++i)
	{
		const Patch *pPatch =texturePatches[i].patch;
		int iXStart = texturePatches[i].dx;
		int iMaxWidth = (iXStart + pPatch->getWidth()) > width ? width : iXStart + pPatch->getWidth();
		for (int iXIndex = (iXStart > 0) ? iXStart : 0; iXIndex < iMaxWidth; iXIndex++)
		{
			numPatchesPerColumn[iXIndex]++;
			columnPatches[iXIndex] = i/*pPatch*/;
			columnIndices[iXIndex] = pPatch->getColumnDataIndex(iXIndex - iXStart);
		}
	}

	for (int i = 0; i < width; ++i)	// Cleanup and update
	{
		if (numPatchesPerColumn[i] > 1)	// Is the column covered by more than one patch?
		{
			columnPatches[i] = -1;
			columnIndices[i] = overlap;
			overlap += height;
		}
	}
	
	overlapColumnData = std::unique_ptr<uint8_t[]>(new uint8_t[overlap]);
	for (int i = 0; i < texturePatches.size(); ++i)
	{
		const Patch *pPatch = texturePatches[i].patch;
		int iXStart = texturePatches[i].dx;
		int iMaxWidth = (iXStart + pPatch->getWidth()) > width ? width : iXStart + pPatch->getWidth();
		for (int iXIndex = (iXStart > 0) ? iXStart : 0; iXIndex < iMaxWidth; iXIndex++)
			if (columnPatches[iXIndex] < 0) // Does this column have more than one patch? if yes compose it, else skip it
				pPatch->composeColumn(overlapColumnData.get() + columnIndices[iXIndex], height, pPatch->getColumnDataIndex(iXIndex - iXStart), texturePatches[i].dy);
	}
}

void Texture::render(uint8_t *buf, int rowlen, int screenx, int screeny)
{
	buf += rowlen * screeny + screenx;
    for (int column = 0; column < width; ++column) renderColumn(buf + column, rowlen, column);
}

void Texture::renderColumn(uint8_t *buf, int rowlen, int column)
{
    if (columnPatches[column] > -1 )
		texturePatches[columnPatches[column]].patch->renderColumn(buf, rowlen, columnIndices[column], height, texturePatches[columnPatches[column]].dy);
    else
        for (int y = 0; y < height; y++, buf += rowlen) *buf = overlapColumnData[columnIndices[column] + y];
}
