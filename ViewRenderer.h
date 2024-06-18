#pragma once

#include <list>
#include "DataTypes.hpp"

class ViewRenderer
{
public:
    ViewRenderer(class Map *map, int renderXSize, int renderYSize);
	~ViewRenderer() {}
    void render(uint8_t *pScreenBuffer, int iBufferPitch, const Viewpoint &v);
	void addWallInFOV(Seg &seg, const Viewpoint &v);

protected:
    struct SolidSegmentRange { int XStart, XEnd; };

    void storeWallRange(Seg &seg, int V1XScreen, int V2XScreen, float V1Angle, float V2Angle, const Viewpoint &v);
	class Map *map;
    int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;

	struct renderLater {const Texture *texture; int x; int from; int to; float u; int cl; int fl;};
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
	std::vector<renderLater> renderLaters;
	uint8_t *screenBuffer;
	int rowlen;
};
