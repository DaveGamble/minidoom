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

    void storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2, const Viewpoint &v);
	void addThing(const Thing &thing, const Viewpoint &v, const Seg &seg);

	int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;
	float invRenderWidth, invRenderHeight;

	struct renderMark {int from, to; float zfrom, zto; };
	std::vector<std::vector<renderMark>> renderMarks;
	void mark(int x, int from, int to, float zfrom, float zto) { renderMarks[x].push_back({from, to, zfrom, zto}); }
	struct renderLater {const Patch *patch; int column, from, to; float v, dv, z; const uint8_t *light;};
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
	std::vector<std::vector<renderLater>> renderLaters;
	const uint8_t (&lights)[34][256];
	uint8_t *screenBuffer;
	int rowlen, frame {0}, texframe {0};
};
