#include "Map.h"

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
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector))
	{
		WADSector *wadsector = (WADSector*)(ptr + i);
		char floorname[9] {}, ceilname[9] {};
		memcpy(floorname, wadsector->FloorTexture, 8);
		memcpy(ceilname, wadsector->CeilingTexture, 8);
		m_Sectors.push_back({wadsector->FloorHeight, wadsector->CeilingHeight, wad->GetTexture(floorname), wad->GetTexture(ceilname), wadsector->Lightlevel, wadsector->Type, wadsector->Tag});
	}
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef))
	{
		WADSidedef *wadsidedef = (WADSidedef*)(ptr + i);
		char uname[9] {}, lname[9] {}, mname[9] {};
		memcpy(uname, wadsidedef->UpperTexture, 8);
		memcpy(lname, wadsidedef->LowerTexture, 8);
		memcpy(mname, wadsidedef->MiddleTexture, 8);
		m_Sidedefs.push_back({wadsidedef->XOffset, wadsidedef->YOffset, wad->GetTexture(uname), wad->GetTexture(mname), wad->GetTexture(lname), m_Sectors.data() + wadsidedef->SectorID});
	}
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef))
	{
		WADLinedef *wadlinedef = (WADLinedef*)(ptr + i);
		m_Linedefs.push_back({m_Vertexes[wadlinedef->StartVertexID], m_Vertexes[wadlinedef->EndVertexID], wadlinedef->Flags, wadlinedef->LineType, wadlinedef->SectorTag,
			(wadlinedef->RightSidedef == 0xFFFF) ? nullptr : m_Sidedefs.data() + wadlinedef->RightSidedef,
			(wadlinedef->LeftSidedef == 0xFFFF) ? nullptr : m_Sidedefs.data() + wadlinedef->LeftSidedef
		});
	}
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg))
	{
		WADSeg *wadseg = (WADSeg*)(ptr + i);
		const Linedef *pLinedef = &m_Linedefs[wadseg->LinedefID];
		const Sidedef *pRightSidedef = wadseg->Direction ? pLinedef->pLeftSidedef : pLinedef->pRightSidedef;
		const Sidedef *pLeftSidedef = wadseg->Direction ? pLinedef->pRightSidedef : pLinedef->pLeftSidedef;
		m_Segs.push_back({m_Vertexes[wadseg->StartVertexID], m_Vertexes[wadseg->EndVertexID], wadseg->SlopeAngle * 360.f / 65536.f, pLinedef, wadseg->Direction, wadseg->Offset / 65536.f,
			(pRightSidedef) ? pRightSidedef->pSector : nullptr, (pLeftSidedef) ? pLeftSidedef->pSector : nullptr});
	}
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
	Angle m_HalfFOV = 45, m_Angle = m_pPlayer->GetAngle();
	int px = m_pPlayer->GetXPosition(), py = m_pPlayer->GetYPosition();
	for (int i = 0; i < subsector.SegCount; i++)
	{
		Seg &seg = m_Segs[subsector.FirstSegID + i];
		Angle V1Angle = Angle(atan2f(seg.start.y - py, seg.start.x - px) * 180.0f / PI);
		Angle V2Angle = Angle(atan2f(seg.end.y - py, seg.end.x - px) * 180.0f / PI);
		Angle V1ToV2Span = V1Angle - V2Angle;

		if (V1ToV2Span >= 180) continue;
		Angle V1AngleFromPlayer = V1Angle - m_Angle; // Rotate every thing.
		Angle V2AngleFromPlayer = V2Angle - m_Angle;
		if (V1AngleFromPlayer + m_HalfFOV > m_FOV)
		{
			if (V1AngleFromPlayer - m_HalfFOV >= V1ToV2Span) continue; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
			V1AngleFromPlayer = m_HalfFOV; // At this point V2 or part of the line should be in the FOV. We need to clip the V1
		}
		if (m_HalfFOV - V2AngleFromPlayer > m_FOV) V2AngleFromPlayer = -m_HalfFOV; // Validate and Clip V2 // Is V2 outside the FOV?
		m_pViewRenderer->AddWallInFOV(seg, V1Angle, V2Angle, V1AngleFromPlayer + 90, V2AngleFromPlayer + 90);
	}
}
