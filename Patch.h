#pragma once
#include <memory>
#include <vector>

#include "DataTypes.h"

class Patch
{
public:
	Patch(const uint8_t *ptr);
	~Patch();

    void Initialize(WADPatchHeader &PatchHeader);
    void Render(uint8_t *pScreenBuffer, int iBufferPitch, int iXScreenLocation, int iYScreenLocation);
    void RenderColumn(uint8_t *pScreenBuffer, int iBufferPitch, int iColumn, int iXScreenLocation, int iYScreenLocation, int iMaxHeight, int iYOffset);
    void ComposeColumn(uint8_t *m_pOverLapColumnData, int m_iHeight, int &iPatchColumnIndex, int iColumnOffsetIndex, int iYOrigin);

	int GetHeight() const { return m_iHeight; }
	int GetWidth() const { return m_iWidth; }
	int GetXOffset() const { return m_iXOffset; }
	int GetYOffset() const { return m_iYOffset; }
	int GetColumnDataIndex(int iIndex) const { return m_ColumnIndex[iIndex]; }

protected:
	int m_iHeight {0}, m_iWidth {0}, m_iXOffset {0}, m_iYOffset {0};

    std::vector<PatchColumnData> m_PatchData;
    std::vector<int> m_ColumnIndex;
};

