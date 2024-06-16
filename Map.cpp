#include "Map.h"

Map::Map(ViewRenderer *pViewRenderer, const std::string &sName, Player *pPlayer, Things *pThings, WADLoader *wad)
: m_pPlayer(pPlayer), m_pThings(pThings), m_pViewRenderer(pViewRenderer)
{
	int li = wad->findLumpByName(sName);
	std::vector<uint8_t> data;
	const uint8_t *ptr;
	size_t size;
	auto seek = [&](const std::string& name) {
		data = wad->getLumpNamed(name, li);
		ptr = data.data();
		size = data.size();
		return ptr;
	};
	
	std::vector<Vertex> vertices;
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) vertices.push_back(*(Vertex*)(ptr + i));

	struct WADSector { int16_t fh, ch; char floorTexture[8], ceilingTexture[8]; uint16_t lightlevel, type, tag; };
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector))
	{
		WADSector *ws = (WADSector*)(ptr + i);
		char floorname[9] {}, ceilname[9] {};
		memcpy(floorname, ws->floorTexture, 8);
		memcpy(ceilname, ws->ceilingTexture, 8);
		m_Sectors.push_back({ws->fh, ws->ch, wad->getTexture(floorname), wad->getTexture(ceilname), ws->lightlevel, ws->type, ws->tag});
	}

	struct WADSidedef { int16_t dx, dy; char upperTexture[8], lowerTexture[8], middleTexture[8]; uint16_t sector; };
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef))
	{
		WADSidedef *ws = (WADSidedef*)(ptr + i);
		char uname[9] {}, lname[9] {}, mname[9] {};
		memcpy(uname, ws->upperTexture, 8);
		memcpy(lname, ws->lowerTexture, 8);
		memcpy(mname, ws->middleTexture, 8);
		m_Sidedefs.push_back({ws->dx, ws->dy, wad->getTexture(uname), wad->getTexture(mname), wad->getTexture(lname), m_Sectors.data() + ws->sector});
	}

	struct WADLinedef { uint16_t start, end, flags, type, sectorTag, rSidedef, lSidedef; }; // Sidedef 0xFFFF means there is no sidedef
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef))
	{
		WADLinedef *wl = (WADLinedef*)(ptr + i);
		m_Linedefs.push_back({vertices[wl->start], vertices[wl->end], wl->flags, wl->type, wl->sectorTag,
			(wl->rSidedef == 0xFFFF) ? nullptr : m_Sidedefs.data() + wl->rSidedef,
			(wl->lSidedef == 0xFFFF) ? nullptr : m_Sidedefs.data() + wl->lSidedef
		});
	}

	struct WADSeg { uint16_t start, end, slopeAngle, linedef, dir, offset; }; // Direction: 0 same as linedef, 1 opposite of linedef Offset: distance along linedef to start of seg
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg))
	{
		WADSeg *ws = (WADSeg*)(ptr + i);
		const Linedef *pLinedef = &m_Linedefs[ws->linedef];
		const Sidedef *pRightSidedef = ws->dir ? pLinedef->lSidedef : pLinedef->rSidedef;
		const Sidedef *pLeftSidedef = ws->dir ? pLinedef->rSidedef : pLinedef->lSidedef;
		m_Segs.push_back({vertices[ws->start], vertices[ws->end], ws->slopeAngle * 360.f / 65536.f, pLinedef, ws->dir, ws->offset / 65536.f,
			(pRightSidedef) ? pRightSidedef->sector : nullptr, (pLeftSidedef) ? pLeftSidedef->sector : nullptr});
	}
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(Thing)) m_pThings->Add(*(Thing*)(ptr + i));
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) m_Nodes.push_back(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) m_Subsector.push_back(*(Subsector*)(ptr + i));
}

void Map::RenderBSPNodes(int iNodeID)
{
    if (!(iNodeID & SUBSECTORIDENTIFIER)) // Masking all the bits exipt the last one to check if this is a subsector
	{
		if (IsPointOnLeftSide(m_pPlayer->getX(), m_pPlayer->getY(), iNodeID))
		{
			RenderBSPNodes(m_Nodes[iNodeID].lChild);
			RenderBSPNodes(m_Nodes[iNodeID].rChild);
		}
		else
		{
			RenderBSPNodes(m_Nodes[iNodeID].rChild);
			RenderBSPNodes(m_Nodes[iNodeID].lChild);
		}
		return;
	}

	Subsector &subsector = m_Subsector[iNodeID & (~SUBSECTORIDENTIFIER)];
	
	constexpr int m_FOV = 90;
	Angle m_HalfFOV = 45, m_Angle = m_pPlayer->getAngle();
	int px = m_pPlayer->getX(), py = m_pPlayer->getY();
	for (int i = 0; i < subsector.numSegs; i++)
	{
		Seg &seg = m_Segs[subsector.firstSeg + i];
		Angle V1Angle = Angle(atan2f(seg.start.y - py, seg.start.x - px) * 180.0f / M_PI);
		Angle V2Angle = Angle(atan2f(seg.end.y - py, seg.end.x - px) * 180.0f / M_PI);
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
