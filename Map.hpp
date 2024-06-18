#pragma once

#include "DataTypes.hpp"
#include <vector>
#include <string>

class Map
{
public:
    Map(const std::string &mapName, class WADLoader& wad);
	~Map() {}

	void render3DView(const Viewpoint &v, class ViewRenderer *render) { renderBSPNodes((int)nodes.size() - 1, v, render); }

    int getPlayerSubSectorHeight(const Viewpoint &v)
	{
		int subsector = (int)(nodes.size() - 1);
		while (!(subsector & kSubsectorIdentifier))
		{
			if (isPointOnLeftSide(v, subsector))
				subsector = nodes[subsector].lChild;
			else
				subsector = nodes[subsector].rChild;
		}
		return segs[subsectors[subsector & (~kSubsectorIdentifier)].firstSeg].rSector->floorHeight;
	}

	const Thing* getThing(int id) { for (auto& t : things) if (t.type == id) return &t; return nullptr; }

protected:
	static constexpr uint16_t kSubsectorIdentifier = 0x8000; // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

    void renderBSPNodes(int iNodeID, const Viewpoint &v, class ViewRenderer *render);

    bool isPointOnLeftSide(const Viewpoint &v, int node) const
	{
		return ((((v.x - nodes[node].x) * nodes[node].dy) - ((v.y - nodes[node].y) * nodes[node].dx)) <= 0);
	}

	std::vector<Thing> things;
    std::vector<Sector> sectors;
    std::vector<Sidedef> sidedefs;
    std::vector<Linedef> linedefs;
    std::vector<Seg> segs;
    std::vector<Subsector> subsectors;
    std::vector<Node> nodes;
};
