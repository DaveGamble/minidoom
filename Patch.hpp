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
			index.push_back((int)cols.size());
			PatchColumnData patchColumn;
			do
			{
				patchColumn.top = ptr[off++];
				if (patchColumn.top != 0xFF)
				{
					patchColumn.length = ptr[off++];
					patchColumn.paddingPre = ptr[off++];
					patchColumn.data = new uint8_t[patchColumn.length];
					memcpy(patchColumn.data, ptr + off, patchColumn.length);
					off += patchColumn.length;
					patchColumn.paddingPost = ptr[off++];
				}
				cols.push_back(patchColumn);
			} while (patchColumn.top != 0xFF);
		}
	}

	~Patch()
	{
		for (size_t i = 0; i < cols.size(); ++i)
			if (cols[i].top != 0xFF) delete[] cols[i].data;
	}


    void render(uint8_t *buf, int rowlen, int screenx, int screeny, const uint8_t *lut, float scale = 1) const
	{
		buf += rowlen * screeny + screenx;
		for (int x = 0, tox = 0; x < width; x++) while (tox < (x + 1) * scale) renderColumn(buf + tox++, rowlen, index[x], INT_MAX, 0, scale, lut);
	}
    void renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset, float scale, const uint8_t *lut) const
	{
		if (scale < 0) return;
		while (cols[firstColumn].top != 0xFF)
		{
			int y = (cols[firstColumn].top + yOffset < 0) ? - cols[firstColumn].top - yOffset : 0;
			int sl = floor(scale * (cols[firstColumn].top + y + yOffset));
			int el = std::min(floor((cols[firstColumn].length - y) * scale) + sl, (float)maxHeight);
			int run = std::max(el - sl, 0), start = rowlen * sl;
			const uint8_t *from = cols[firstColumn].data + y;
			for (int i = 0, to = 0; to < run && i < cols[firstColumn].length; i++)
				while (to < (i + 1) * scale && to < run) buf[start + (to++) * rowlen] = lut[from[i]];
			++firstColumn;
		}
	}
	uint16_t pixel(int x, int y) const
	{
		for ( ; cols[x].top != 0xff && cols[x].top <= y; x++) { int o = y - cols[x].top; if (o >= 0 && o < cols[x].length) return cols[x].data[o]; } return 256;
	}
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset) const
	{
		while (cols[firstColumn].top != 0xFF)
		{
			int y = yOffset + cols[firstColumn].top, iMaxRun = cols[firstColumn].length;
			if (y < 0) { iMaxRun += y; y = 0; }
			if (iMaxRun > iHeight - y) iMaxRun = iHeight - y;
			if (iMaxRun > 0) memcpy(buf + y, cols[firstColumn].data, iMaxRun);
			++firstColumn;
		}
	}

	int getWidth() const { return width; }
	int getHeight() const { return height; }
	int getXOffset() const { return xoffset; }
	int getYOffset() const { return yoffset; }
	int getColumnDataIndex(int x) const { return index[x]; }

protected:
	int height {0}, width {0}, xoffset {0}, yoffset {0};
	struct PatchColumnData { uint8_t top, length, paddingPre, *data, paddingPost; };

    std::vector<PatchColumnData> cols;
    std::vector<int> index;
};

