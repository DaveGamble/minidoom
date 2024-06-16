#pragma once
#include <string>
#include <vector>

#include "DataTypes.hpp"
#include "Patch.h"

class Texture
{
public:
    Texture(WADTextureData &TextureData, class WADLoader *wad);
	~Texture() {}

    void Render(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation);
    void RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation, int iCurrentColumnIndex);
protected:
    int m_iWidth, m_iHeight, m_iOverLapSize {0};

	class WADLoader *wad;
    std::vector<int> m_ColumnPatchCount, m_ColumnIndex, m_ColumnPatch;
    std::vector<WADTexturePatch> m_TexturePatches;
    std::unique_ptr<uint8_t[]> m_pOverLapColumnData;

};
