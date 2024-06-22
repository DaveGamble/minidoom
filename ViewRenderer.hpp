#pragma once

#include <list>
#include "DataTypes.hpp"

class ViewRenderer
{
public:
    ViewRenderer(int renderXSize, int renderYSize, const uint8_t (&lights)[34][256]);
	~ViewRenderer() {}
    void render(uint8_t *pScreenBuffer, int iBufferPitch, const Viewpoint &v, class Map &map);
	void addWallInFOV(const Seg &seg, const Viewpoint &v);

protected:
    struct SolidSegmentRange { int start, end; };

    void storeWallRange(const Seg &seg, int x1, int x2, const Viewpoint &v);

	int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;

	struct renderLater {const Texture *texture; int x; int from, to; float u, v, dv; const uint8_t *light;};
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
	std::vector<renderLater> renderLaters;
	const uint8_t (&lights)[34][256];
	uint8_t *screenBuffer;
	int rowlen;
};
