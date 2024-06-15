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

Patch::Patch(WADPatchHeader &PatchHeader)
{
    m_iWidth = PatchHeader.Width;
    m_iHeight = PatchHeader.Height;
    m_iXOffset = PatchHeader.LeftOffset;
    m_iYOffset = PatchHeader.TopOffset;
}

void Patch::Render(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation)
{
    int iXIndex = 0;
    for (size_t iPatchColumnIndex = 0; iPatchColumnIndex < m_PatchData.size(); iPatchColumnIndex++)
    {
        if (m_PatchData[iPatchColumnIndex].TopDelta == 0xFF) { ++iXIndex; continue; }
        for (int iYIndex = 0; iYIndex < m_PatchData[iPatchColumnIndex].Length; ++iYIndex)
            pScreenBuffer[iBufferPitch * (iYScreenLocation + m_PatchData[iPatchColumnIndex].TopDelta + iYIndex) + (iXScreenLocation + iXIndex)] = m_PatchData[iPatchColumnIndex].pColumnData[iYIndex];
    }
}

void Patch::RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iColumn, int iXScreenLocation, int iYScreenLocation, int iMaxHeight, int iYOffset)
{
    int iTotalHeight = 0, iYIndex = 0;
    if (iYOffset < 0) iYIndex = iYOffset * -1;

    while (m_PatchData[iColumn].TopDelta != 0xFF && iTotalHeight < iMaxHeight)
    {
        while (iYIndex < m_PatchData[iColumn].Length && iTotalHeight < iMaxHeight)
        {
            pScreenBuffer[iBufferPitch * (iYScreenLocation + m_PatchData[iColumn].TopDelta + iYIndex + iYOffset) + iXScreenLocation] = m_PatchData[iColumn].pColumnData[iYIndex];
            ++iTotalHeight;
            ++iYIndex;
        }
        ++iColumn;
        iYIndex = 0;
    }
}

void Patch::ComposeColumn(uint8_t *pOverLapColumnData, int iHeight, int &iPatchColumnIndex, int iColumnOffsetIndex, int iYOrigin)
{
    while (m_PatchData[iPatchColumnIndex].TopDelta != 0xFF)
    {
        int iYPosition = iYOrigin + m_PatchData[iPatchColumnIndex].TopDelta, iMaxRun = m_PatchData[iPatchColumnIndex].Length;

        if (iYPosition < 0) { iMaxRun += iYPosition; iYPosition = 0; }
        if (iYPosition + iMaxRun > iHeight) iMaxRun = iHeight - iYPosition;

        for (int iYIndex = 0; iYIndex < iMaxRun; ++iYIndex)
            pOverLapColumnData[iColumnOffsetIndex + iYPosition + iYIndex] = m_PatchData[iPatchColumnIndex].pColumnData[iYIndex];
        ++iPatchColumnIndex;
    }
}
