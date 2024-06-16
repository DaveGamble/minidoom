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
	
	overLapColumnData = std::unique_ptr<uint8_t[]>(new uint8_t[overlap]);
	for (int i = 0; i < texturePatches.size(); ++i)
	{
		const Patch *pPatch = texturePatches[i].patch;
		int iXStart = texturePatches[i].dx;
		int iMaxWidth = (iXStart + pPatch->getWidth()) > width ? width : iXStart + pPatch->getWidth();
		for (int iXIndex = (iXStart > 0) ? iXStart : 0; iXIndex < iMaxWidth; iXIndex++)
			if (columnPatches[iXIndex] < 0) // Does this column have more than one patch? if yes compose it, else skip it
				pPatch->composeColumn(overLapColumnData.get() + columnIndices[iXIndex], height, pPatch->getColumnDataIndex(iXIndex - iXStart), texturePatches[i].dy);
	}
}

void Texture::render(uint8_t * pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation)
{
    for (int iCurrentColumnIndex = 0; iCurrentColumnIndex < width; ++iCurrentColumnIndex)
        renderColumn(pScreenBuffer, iBufferPitch, iXScreenLocation + iCurrentColumnIndex, iYScreenLocation, iCurrentColumnIndex);
}

void Texture::renderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation, int iCurrentColumnIndex)
{
	pScreenBuffer += iBufferPitch * iYScreenLocation + iXScreenLocation;
    if (columnPatches[iCurrentColumnIndex] > -1 )
    {
		const Patch *pPatch = texturePatches[columnPatches[iCurrentColumnIndex]].patch;
        pPatch->renderColumn(pScreenBuffer, iBufferPitch, columnIndices[iCurrentColumnIndex], height, texturePatches[columnPatches[iCurrentColumnIndex]].dy);
    }
    else
    {
        for (int iYIndex = 0; iYIndex < height; ++iYIndex, pScreenBuffer += iBufferPitch)
            *pScreenBuffer = overLapColumnData[columnIndices[iCurrentColumnIndex] + iYIndex];
    }
}
