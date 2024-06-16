#include "Patch.h"

Patch::Patch(const uint8_t *ptr)
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

Patch::~Patch()
{
	for (size_t i = 0; i < patchColumnData.size(); ++i)
		if (patchColumnData[i].topDelta != 0xFF) delete[] patchColumnData[i].columnData;
}

void Patch::render(uint8_t *buf, int rowlen, int screenx, int screeny, float scale) const
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

void Patch::renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset, float scale) const
{
	if (scale < 0) return;
    int y = (yOffset < 0) ? -yOffset : 0;
    while (patchColumnData[firstColumn].topDelta != 0xFF && maxHeight > 0)
    {
		int run = std::clamp(int((patchColumnData[firstColumn].length - y) * scale), 0, maxHeight);
		int start = rowlen * floor(scale * (patchColumnData[firstColumn].topDelta + y + yOffset));
		const uint8_t *from = patchColumnData[firstColumn].columnData + y;
		for (int i = 0, to = 0; to < run; i++) while (to < (i + 1) * scale) buf[start + (to++) * rowlen] = from[i];
		maxHeight -= run;
        ++firstColumn;
        y = 0;
    }
}

void Patch::composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const
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
