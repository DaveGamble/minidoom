#pragma once
#include "Patch.h"

class Texture
{
public:
    Texture(const uint8_t *ptr, class WADLoader *wad);
	~Texture() {}

    void Render(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation);
    void RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation, int iCurrentColumnIndex);
protected:
    int m_iWidth, m_iHeight, m_iOverLapSize {0};

	class WADLoader *wad;
    std::vector<int> m_ColumnPatchCount, m_ColumnIndex, m_ColumnPatch;
	struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, stepDir, colorMap; }; // StepDir, ColorMap Unused values.
	std::vector<WADTexturePatch> m_TexturePatches;
    std::unique_ptr<uint8_t[]> m_pOverLapColumnData;
};
