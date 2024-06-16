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
	
	constexpr int m_FOV = 90;
	Angle m_HalfFOV = 45;
	Angle m_Angle = m_pPlayer->GetAngle();
	int px = m_pPlayer->GetXPosition(), py = m_pPlayer->GetYPosition();
	for (int i = 0; i < subsector.SegCount; i++)
	{
		Seg &seg = m_Segs[subsector.FirstSegID + i];
		Angle V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer;
		
		V1Angle = Angle(atan2f(seg.pStartVertex.YPosition - py, seg.pStartVertex.XPosition - px) * 180.0f / PI);
		V2Angle = Angle(atan2f(seg.pEndVertex.YPosition - py, seg.pEndVertex.XPosition - px) * 180.0f / PI);
		Angle V1ToV2Span = V1Angle - V2Angle;

		if (V1ToV2Span >= 180) continue;
		V1AngleFromPlayer = V1Angle - m_Angle; // Rotate every thing.
		V2AngleFromPlayer = V2Angle - m_Angle;
			
		Angle V1Moved = V1AngleFromPlayer + m_HalfFOV; // Validate and Clip V1. shift angles to be between 0 and 90 (now virtualy we shifted FOV to be in that range)

		if (V1Moved > m_FOV)
		{
			if (V1Moved - m_FOV >= V1ToV2Span) continue; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
			V1AngleFromPlayer = m_HalfFOV; // At this point V2 or part of the line should be in the FOV. We need to clip the V1
		}
		if (m_HalfFOV - V2AngleFromPlayer > m_FOV) V2AngleFromPlayer = -m_HalfFOV; // Validate and Clip V2 // Is V2 outside the FOV?

		V1AngleFromPlayer += 90;
		V2AngleFromPlayer += 90;
		
		m_pViewRenderer->AddWallInFOV(seg, V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer);
	}
}
