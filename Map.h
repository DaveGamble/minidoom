#pragma once

#include "ViewRenderer.h"
#include "WADLoader.hpp"

class Map
{
public:
    Map(ViewRenderer *renderer, const std::string &mapName, Things *things, WADLoader *wad);
	~Map() {}

	void render3DView(int px, int py, int pz, float pa) { renderBSPNodes((int)nodes.size() - 1, px, py, pz, pa); }

    int getPlayerSubSectorHeight(int px, int py)
	{
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier))
		{
			if (isPointOnLeftSide(px, py, subsector))
				subsector = nodes[subsector].lChild;
			else
				subsector = nodes[subsector].rChild;
		}
		return segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
	}

	Things* getThings() { return things; }

protected:
	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

    void renderBSPNodes(int iNodeID, int px, int py, int pz, float pa);

    bool isPointOnLeftSide(int x, int y, int node) const
	{
		return ((((x - nodes[node].x) * nodes[node].dy) - ((y - nodes[node].y) * nodes[node].dx)) <= 0);
	}

    std::vector<Sector> sectors;
    std::vector<Sidedef> sidedefs;
    std::vector<Linedef> linedefs;
    std::vector<Seg> segs;
    std::vector<Subsector> subsectors;
    std::vector<Node> nodes;
    Things *things;
    ViewRenderer *renderer;
};
