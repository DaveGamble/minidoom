#pragma once

#include "DataTypes.hpp"
#include <vector>
#include <string>

class Map
{
public:
    Map(const std::string &mapName, class WADLoader& wad);
	~Map() {}

	template<typename F> void render3DView(const Viewpoint &v, F &&addWall) const { renderBSPNodes((int)nodes.size() - 1, v, addWall); }

    int getPlayerSubSectorHeight(const Viewpoint &v) const
	{
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier)) subsector = isPointOnLeftSide(v, subsector) ? nodes[subsector].lChild : nodes[subsector].rChild;
		return segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
	}

	const Thing* getThing(int id) const { for (const Thing& t : things) if (t.type == id) return &t; return nullptr; }

protected:
	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

    template<typename F> void renderBSPNodes(int iNodeID, const Viewpoint &v, F &&addWall) const
	{
		if (iNodeID & kSubsectorIdentifier) // Masking all the bits exipt the last one to check if this is a subsector
		{
			iNodeID &= ~kSubsectorIdentifier;
			for (int i = 0; i < subsectors[iNodeID].numSegs; i++) addWall(segs[subsectors[iNodeID].firstSeg + i]);
			return;
		}
		const bool left = isPointOnLeftSide(v, iNodeID);
		renderBSPNodes(left ? nodes[iNodeID].lChild : nodes[iNodeID].rChild, v, addWall);
		renderBSPNodes(left ? nodes[iNodeID].rChild : nodes[iNodeID].lChild, v, addWall);
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
};
