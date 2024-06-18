#pragma once
#include "DataTypes.hpp"
#include <vector>

class Patch
{
public:
	Patch(const uint8_t *ptr)
	{
		struct WADPatchHeader { uint16_t width, height; int16_t leftOffset, topOffset; };

		WADPatchHeader *patchHeader = (WADPatchHeader*)ptr;
		width = patchHeader->width;
		height = patchHeader->height;
		xoffset = patchHeader->leftOffset;
		yoffset = patchHeader->topOffset;

		uint32_t *columnOffsets = (uint32_t*)(ptr + 8);
		for (int i = 0; i < width; ++i)
		{
			int off = columnOffsets[i];
			columnIndices.push_back((int)patchColumnData.size());
			PatchColumnData patchColumn;
			do
			{
				patchColumn.topDelta = ptr[off++];
				if (patchColumn.topDelta != 0xFF)
				{
					patchColumn.length = ptr[off++];
					patchColumn.paddingPre = ptr[off++];
					patchColumn.columnData = new uint8_t[patchColumn.length];
					memcpy(patchColumn.columnData, ptr + off, patchColumn.length);
					off += patchColumn.length;
					patchColumn.paddingPost = ptr[off++];
				}
				patchColumnData.push_back(patchColumn);
			} while (patchColumn.topDelta != 0xFF);
		}
	}

	~Patch()
	{
		for (size_t i = 0; i < patchColumnData.size(); ++i)
			if (patchColumnData[i].topDelta != 0xFF) delete[] patchColumnData[i].columnData;
	}


    void render(uint8_t *buf, int rowlen, int screenx, int screeny, float scale = 1) const
	{
		buf += rowlen * screeny + screenx;
		for (int x = 0, tox = 0; x < width; x++)
		{
			while (tox < (x + 1) * scale)
			{
				for (int column = columnIndices[x]; patchColumnData[column].topDelta != 0xFF; column++)
				{
					int off = floor(scale * patchColumnData[column].topDelta) * rowlen + tox;
					uint8_t *from = patchColumnData[column].columnData;
					for (int y = 0, to = 0; y < patchColumnData[column].length; y++) while (to < (y + 1) * scale) buf[off + (to++) * rowlen] = from[y];
				}
				tox++;
			}
		}
	}
    void renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset, float scale = 1) const
	{
		if (scale < 0) return;
		while (patchColumnData[firstColumn].topDelta != 0xFF)
		{
			int y = (patchColumnData[firstColumn].topDelta + yOffset < 0) ? - patchColumnData[firstColumn].topDelta - yOffset : 0;
			int sl = floor(scale * (patchColumnData[firstColumn].topDelta + y + yOffset));
			int el = std::min(floor((patchColumnData[firstColumn].length - y) * scale) + sl, (float)maxHeight);
			int run = std::max(el - sl, 0);
			int start = rowlen * sl;
			const uint8_t *from = patchColumnData[firstColumn].columnData + y;
			for (int i = 0, to = 0; to < run && i < patchColumnData[firstColumn].length; i++) while (to < (i + 1) * scale && to < run) buf[start + (to++) * rowlen] = from[i];
			++firstColumn;
		}
	}
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const
	{
		while (patchColumnData[firstColumn].topDelta != 0xFF)
		{
			int y = yOffset + patchColumnData[firstColumn].topDelta, iMaxRun = patchColumnData[firstColumn].length;
			if (y < 0) { iMaxRun += y; y = 0; }
			if (iMaxRun > iHeight - y) iMaxRun = iHeight - y;
			if (iMaxRun > 0) memcpy(buf + y, patchColumnData[firstColumn].columnData, iMaxRun);
			++firstColumn;
		}
	}

	int getWidth() const { return width; }
	int getXOffset() const { return xoffset; }
	int getYOffset() const { return yoffset; }
	int getColumnDataIndex(int iIndex) const { return columnIndices[iIndex]; }

protected:
	int height {0}, width {0}, xoffset {0}, yoffset {0};
	struct PatchColumnData { uint8_t topDelta, length, paddingPre, *columnData, paddingPost; };

    std::vector<PatchColumnData> patchColumnData;
    std::vector<int> columnIndices;
};
