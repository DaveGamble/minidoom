#include "Patch.h"

using namespace std;


Patch::~Patch()
{
    for (size_t iPatchColumnIndex = 0; iPatchColumnIndex < m_PatchData.size(); ++iPatchColumnIndex)
    {
        if (m_PatchData[iPatchColumnIndex].TopDelta == 0xFF) continue;
        delete[] m_PatchData[iPatchColumnIndex].pColumnData;
        m_PatchData[iPatchColumnIndex].pColumnData = nullptr;
    }
}

Patch::Patch(const uint8_t *ptr)
{
	WADPatchHeader *PatchHeader = (WADPatchHeader*)ptr;
	m_iWidth = PatchHeader->Width;
	m_iHeight = PatchHeader->Height;
	m_iXOffset = PatchHeader->LeftOffset;
	m_iYOffset = PatchHeader->TopOffset;

	uint32_t *pColumnOffsets = (uint32_t*)(ptr + 8);
	for (int i = 0; i < m_iWidth; ++i)
	{
		int Offset = pColumnOffsets[i];
		m_ColumnIndex.push_back((int)m_PatchData.size());
		PatchColumnData PatchColumn;
		do
		{
			PatchColumn.TopDelta = ptr[Offset++];
			if (PatchColumn.TopDelta != 0xFF)
			{
				PatchColumn.Length = ptr[Offset++];
				PatchColumn.PaddingPre = ptr[Offset++];
				PatchColumn.pColumnData = new uint8_t[PatchColumn.Length];
				memcpy(PatchColumn.pColumnData, ptr + Offset, PatchColumn.Length);
				Offset += PatchColumn.Length;
				PatchColumn.PaddingPost = ptr[Offset++];
			}
			m_PatchData.push_back(PatchColumn);
		} while (PatchColumn.TopDelta != 0xFF);
	}
}

void Patch::Render(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation)
{
	int offset = iBufferPitch * iYScreenLocation + iXScreenLocation;
    for (size_t iPatchColumnIndex = 0; iPatchColumnIndex < m_PatchData.size(); iPatchColumnIndex++)
    {
		int off = offset + m_PatchData[iPatchColumnIndex].TopDelta * iBufferPitch;
        if (m_PatchData[iPatchColumnIndex].TopDelta == 0xFF) offset++;
		else
			for (int y = 0; y < m_PatchData[iPatchColumnIndex].Length; y++, off += iBufferPitch)
				pScreenBuffer[off] = m_PatchData[iPatchColumnIndex].pColumnData[y];
    }
}

void Patch::RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iColumn, int iXScreenLocation, int iYScreenLocation, int iMaxHeight, int iYOffset)
{
    int iYIndex = (iYOffset < 0) ? iYOffset * -1 : 0;
    while (m_PatchData[iColumn].TopDelta != 0xFF && iMaxHeight > 0)
    {
		int run = m_PatchData[iColumn].Length - iYIndex;
		run = (run > iMaxHeight) ? iMaxHeight : run;
		if (run > 0)
		{
			memcpy(pScreenBuffer + iBufferPitch * (iYScreenLocation + m_PatchData[iColumn].TopDelta + iYIndex + iYOffset) + iXScreenLocation, m_PatchData[iColumn].pColumnData + iYIndex, run);
			iMaxHeight -= run;
		}
        ++iColumn;
        iYIndex = 0;
    }
}

void Patch::ComposeColumn(uint8_t *pOverLapColumnData, int iHeight, int &iPatchColumnIndex, int iYOrigin)
{
    while (m_PatchData[iPatchColumnIndex].TopDelta != 0xFF)
    {
        int iYPosition = iYOrigin + m_PatchData[iPatchColumnIndex].TopDelta, iMaxRun = m_PatchData[iPatchColumnIndex].Length;
        if (iYPosition < 0) { iMaxRun += iYPosition; iYPosition = 0; }
        if (iMaxRun > iHeight - iYPosition) iMaxRun = iHeight - iYPosition;
		if (iMaxRun > 0) memcpy(pOverLapColumnData + iYPosition, m_PatchData[iPatchColumnIndex].pColumnData, iMaxRun);
        ++iPatchColumnIndex;
    }
}
