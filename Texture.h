#pragma once
#include "Patch.h"

class Texture
{
public:
    Texture(const uint8_t *ptr, class WADLoader *wad);
	~Texture() {}

    void render(uint8_t *buf, int rowlen, int screenx, int screeny);
    void renderColumn(uint8_t *buf, int rowlen, int firstColumn);
protected:
    int width, height, overlap {0};

    std::vector<int> columnIndices, columnYOffsets;
	std::vector<const Patch*> columnPatches;
	struct TexturePatch { int16_t dx, dy; uint16_t stepDir, colorMap; const Patch *patch; }; // StepDir, ColorMap Unused values.
	std::vector<TexturePatch> texturePatches;
    std::unique_ptr<uint8_t[]> overlapColumnData;
};
