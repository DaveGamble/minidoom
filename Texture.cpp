#include "Texture.h"
#include "WADLoader.hpp"

#include <string>
using namespace std;

Texture::Texture(WADTextureData &TextureData, WADLoader *_wad) : wad(_wad)
{
	m_iWidth = TextureData.Width;
	m_iHeight = TextureData.Height;
	
	for (int i = 0; i < TextureData.PatchCount; ++i)
		m_TexturePatches.push_back(TextureData.pTexturePatch[i]);
	
	m_ColumnPatchCount.resize(TextureData.Width, 0);
	m_ColumnIndex.resize(TextureData.Width, 0);
	m_ColumnPatch.resize(TextureData.Width, 0);

	for (int i = 0; i < m_TexturePatches.size(); ++i)
	{
		Patch *pPatch = wad->GetPatch(m_TexturePatches[i].PNameIndex);
		int iXStart = m_TexturePatches[i].XOffset;
		int iMaxWidth = (iXStart + pPatch->GetWidth()) > m_iWidth ? m_iWidth : iXStart + pPatch->GetWidth();
		for (int iXIndex = (iXStart > 0) ? iXStart : 0; iXIndex < iMaxWidth; iXIndex++)
		{
			m_ColumnPatchCount[iXIndex]++;
			m_ColumnPatch[iXIndex] = i/*pPatch*/;
			m_ColumnIndex[iXIndex] = pPatch->GetColumnDataIndex(iXIndex - iXStart);
		}
	}

	for (int i = 0; i < m_iWidth; ++i)	// Cleanup and update
	{
		if (m_ColumnPatchCount[i] > 1)	// Is the column covered by more than one patch?
		{
			m_ColumnPatch[i] = -1;
			m_ColumnIndex[i] = m_iOverLapSize;
			m_iOverLapSize += m_iHeight;
		}
	}
	
	m_pOverLapColumnData = std::unique_ptr<uint8_t[]>(new uint8_t[m_iOverLapSize]);
	for (int i = 0; i < m_TexturePatches.size(); ++i)
	{
		Patch *pPatch = wad->GetPatch(m_TexturePatches[i].PNameIndex);
		int iXStart = m_TexturePatches[i].XOffset;
		int iMaxWidth = (iXStart + pPatch->GetWidth()) > m_iWidth ? m_iWidth : iXStart + pPatch->GetWidth();
		for (int iXIndex = (iXStart > 0) ? iXStart : 0; iXIndex < iMaxWidth; iXIndex++)
			if (m_ColumnPatch[iXIndex] < 0) // Does this column have more than one patch? if yes compose it, else skip it
				pPatch->ComposeColumn(m_pOverLapColumnData.get() + m_ColumnIndex[iXIndex], m_iHeight, pPatch->GetColumnDataIndex(iXIndex - iXStart), m_TexturePatches[i].YOffset);
	}
}

void Texture::Render(uint8_t * pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation)
{
    for (int iCurrentColumnIndex = 0; iCurrentColumnIndex < m_iWidth; ++iCurrentColumnIndex)
        RenderColumn(pScreenBuffer, iBufferPitch, iXScreenLocation + iCurrentColumnIndex, iYScreenLocation, iCurrentColumnIndex);
}

void Texture::RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation, int iCurrentColumnIndex)
{
	pScreenBuffer += iBufferPitch * iYScreenLocation + iXScreenLocation;
    if (m_ColumnPatch[iCurrentColumnIndex] > -1 )
    {
        Patch *pPatch = wad->GetPatch(m_TexturePatches[m_ColumnPatch[iCurrentColumnIndex]].PNameIndex);
        pPatch->RenderColumn(pScreenBuffer, iBufferPitch, m_ColumnIndex[iCurrentColumnIndex], m_iHeight, m_TexturePatches[m_ColumnPatch[iCurrentColumnIndex]].YOffset);
    }
    else
    {
        for (int iYIndex = 0; iYIndex < m_iHeight; ++iYIndex, pScreenBuffer += iBufferPitch)
            *pScreenBuffer = m_pOverLapColumnData[m_ColumnIndex[iCurrentColumnIndex] + iYIndex];
    }
}
