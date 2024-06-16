#pragma once

#include "Player.hpp"
#include "ViewRenderer.h"
#include "WADLoader.hpp"

class Map
{
public:
    Map(ViewRenderer *pViewRenderer, const std::string &sName, Player *pPlayer, Things *pThings, WADLoader *w);
	~Map() {}

	void Render3DView() { RenderBSPNodes((int)m_Nodes.size() - 1); }

    int GetPlayerSubSectorHeight()
	{
		int iSubsectorID = (int)(m_Nodes.size() - 1);
		while (!(iSubsectorID & SUBSECTORIDENTIFIER))
		{
			if (IsPointOnLeftSide(m_pPlayer->getX(), m_pPlayer->getY(), iSubsectorID))
				iSubsectorID = m_Nodes[iSubsectorID].lChild;
			else
				iSubsectorID = m_Nodes[iSubsectorID].rChild;
		}
		return m_Segs[m_Subsector[iSubsectorID & (~SUBSECTORIDENTIFIER)].firstSeg].rSector->floorHeight;
	}

	Things* GetThings() { return m_pThings; }

protected:
    void RenderBSPNodes(int iNodeID);

    bool IsPointOnLeftSide(int XPosition, int YPosition, int iNodeID) const
	{
		return ((((XPosition - m_Nodes[iNodeID].x) * m_Nodes[iNodeID].dy) - ((YPosition - m_Nodes[iNodeID].y) * m_Nodes[iNodeID].dx)) <= 0);
	}

    std::vector<Sector> m_Sectors;
    std::vector<Sidedef> m_Sidedefs;
    std::vector<Linedef> m_Linedefs;
    std::vector<Seg> m_Segs;
    std::vector<Subsector> m_Subsector;
    std::vector<Node> m_Nodes;
    Player *m_pPlayer;
    Things *m_pThings;
    ViewRenderer *m_pViewRenderer;
};
