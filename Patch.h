#pragma once
#include "DataTypes.hpp"

class Patch
{
public:
	Patch(const uint8_t *ptr);
	~Patch();

    void render(uint8_t *buf, int rowlen, int screenx, int screeny);
    void renderColumn(uint8_t *buf, int rowlen, int firstColumn, int maxHeight, int yOffset);
	void composeColumn(uint8_t *buf, int iHeight, int firstColumn, int yOffset);

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

