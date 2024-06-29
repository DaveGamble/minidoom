#pragma once

#include <list>
#include "WADLoader.hpp"

class ViewRenderer
{
public:
    ViewRenderer(int renderXSize, int renderYSize, const char *wadname, const char *mapName);
	~ViewRenderer() {}

	void render(uint8_t *pScreenBuffer, int iBufferPitch, const Viewpoint &v);

	WADLoader &getWad() {return wad;}
	const Thing* getThing(int id) const { for (const Thing& t : things) if (t.type == id) return &t; return nullptr; }

	bool doesLineIntersect(int x1, int y1, int x2, int y2) const
	{
		std::vector<const Linedef *> tests = getBlock(x1, y1);
		if (x1 >> 7 != x2 >> 7 || y1 >> 7 != y2 >> 7)
		{
			std::vector<const Linedef *> tests2 = getBlock(x2, y2);
			tests.insert(tests.end(), tests2.begin(), tests2.end());
		}
		for (const Linedef *l : tests)	// Graphics Gems 3.
		{
			if (l->lSidedef && !(l->flags & 1)) continue;	// Could test for doors here.
			const float Ax = x2 - x1, Ay = y2 - y1, Bx = l->start.x - l->end.x, By = l->start.y - l->end.y, Cx = x1 - l->start.x, Cy = y1 - l->start.y;
			float den = Ay * Bx - Ax * By, tn = By * Cx - Bx * Cy;
			if (den > 0) { if (tn < 0 || tn > den) continue; } else if (tn > 0 || tn < den) continue;
			float un = Ax * Cy - Ay * Cx;
			if (den > 0) { if (un < 0 || un > den) continue; } else if (un > 0 || un < den) continue;
			return true;
		}
		// Test thing collisions here?
		return false;
	}

	int getPlayerSubSectorHeight(const Viewpoint &v) const
	{
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier)) subsector = isPointOnLeftSide(v, subsector) ? nodes[subsector].lChild : nodes[subsector].rChild;
		return segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
	}
protected:
	void addWallInFOV(const Seg &seg, const Viewpoint &v);

	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

	void renderBSPNodes(int iNodeID, const Viewpoint &v)
	{
		if (iNodeID & kSubsectorIdentifier) // Masking all the bits exipt the last one to check if this is a subsector
		{
			iNodeID &= ~kSubsectorIdentifier;
			for (int i = 0; i < subsectors[iNodeID].numSegs; i++) addWallInFOV(segs[subsectors[iNodeID].firstSeg + i], v);
			return;
		}
		const bool left = isPointOnLeftSide(v, iNodeID);
		renderBSPNodes(left ? nodes[iNodeID].lChild : nodes[iNodeID].rChild, v);
		renderBSPNodes(left ? nodes[iNodeID].rChild : nodes[iNodeID].lChild, v);
	}
	
	std::vector<const Linedef *> getBlock(int x, int y) const
	{
		if (y < blockmap_y || ((y - blockmap_y) >> 7) >= blockmap.size()) return {};
		if (x < blockmap_x || ((x - blockmap_x) >> 7) >= blockmap[0].size()) return {};
		return blockmap[(y - blockmap_y) >> 7][(x - blockmap_x) >> 7];
	}

	bool isPointOnLeftSide(const Viewpoint &v, int node) const
	{
		return ((((v.x - nodes[node].x) * nodes[node].dy) - ((v.y - nodes[node].y) * nodes[node].dx)) <= 0);
	}
	
	struct Subsector { uint16_t numSegs, firstSeg; };
	struct BBox { int16_t top, bottom, left, right; };
	struct Node { int16_t x, y, dx, dy; BBox rBox, lBox; uint16_t rChild, lChild; };

	std::vector<Thing> things;
	std::vector<Sector> sectors;
	std::vector<Sidedef> sidedefs;
	std::vector<Linedef> linedefs;
	std::vector<Seg> segs;
	std::vector<Subsector> subsectors;
	std::vector<Node> nodes;
	
	std::vector<std::vector<std::vector<const Linedef *>>> blockmap;
	int16_t blockmap_x, blockmap_y;
	
    struct SolidSegmentRange { int start, end; };

    void storeWallRange(const Seg &seg, int x1, int x2, float ux1, float ux2, float z1, float z2, const Viewpoint &v);
	void addThing(const Thing &thing, const Viewpoint &v, const Seg &seg);

	int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;
	float invRenderWidth, invRenderHeight;

	struct renderMark {int from, to; float zfrom, zto; };
	std::vector<std::vector<renderMark>> renderMarks;
	void mark(int x, int from, int to, float zfrom, float zto) { if (to >= from) renderMarks[x].push_back({from, to, zfrom, zto}); }
	struct renderLater {const Patch *patch; int column, from, to; float v, dv, z; const uint8_t *light;};
    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
	std::vector<std::vector<renderLater>> renderLaters;
	uint8_t lights[34][256];
	uint8_t *screenBuffer;
	int rowlen, frame {0}, texframe {0};
	
	WADLoader wad;
};
