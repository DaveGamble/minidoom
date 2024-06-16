#include "Map.h"

#include <stdlib.h> 

using namespace std;

Map::Map(ViewRenderer *pViewRenderer, const std::string &sName, Player *pPlayer, Things *pThings, WADLoader *wad)
: m_pPlayer(pPlayer), m_pThings(pThings), m_pViewRenderer(pViewRenderer)
{
	int li = wad->FindLumpByName(sName);
	std::vector<uint8_t> data;
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const std::string& name) {
		data = wad->GetLumpNamed(name, li);
		ptr = data.data();
		size = data.size();
		return ptr;
	};
	
	std::vector<Vertex> m_Vertexes;
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) m_Vertexes.push_back(*(Vertex*)(ptr + i));
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector)) m_Sectors.push_back(*(WADSector*)(ptr + i));
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef)) m_Sidedefs.push_back(Sidedef(*(WADSidedef*)(ptr + i), m_Sectors));
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef)) m_Linedefs.push_back(Linedef(*(WADLinedef*)(ptr + i), m_Sidedefs, m_Vertexes));
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg)) m_Segs.push_back(Seg(*(WADSeg*)(ptr + i), m_Linedefs, m_Vertexes));
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(Thing)) m_pThings->AddThing(*(Thing*)(ptr + i));
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) m_Nodes.push_back(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) m_Subsector.push_back(*(Subsector*)(ptr + i));
}

void Map::RenderBSPNodes(int iNodeID)
{
    if (!(iNodeID & SUBSECTORIDENTIFIER)) // Masking all the bits exipt the last one to check if this is a subsector
	{
		if (IsPointOnLeftSide(m_pPlayer->GetXPosition(), m_pPlayer->GetYPosition(), iNodeID))
		{
			RenderBSPNodes(m_Nodes[iNodeID].LeftChildID);
			RenderBSPNodes(m_Nodes[iNodeID].RightChildID);
		}
		else
		{
			RenderBSPNodes(m_Nodes[iNodeID].RightChildID);
			RenderBSPNodes(m_Nodes[iNodeID].LeftChildID);
		}
		return;
	}

	Subsector &subsector = m_Subsector[iNodeID & (~SUBSECTORIDENTIFIER)];
	
	for (int i = 0; i < subsector.SegCount; i++)
	{
		Seg &seg = m_Segs[subsector.FirstSegID + i];
		Angle V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer;
		if (m_pPlayer->ClipVertexesInFOV((seg.pStartVertex), (seg.pEndVertex), V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer))
			m_pViewRenderer->AddWallInFOV(seg, V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer);
	}
}
