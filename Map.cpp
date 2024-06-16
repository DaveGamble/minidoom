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
	
	if (seek("VERTEXES")) for (int i = 0; i < size; i += sizeof(Vertex)) m_Vertexes.push_back(*(Vertex*)(ptr + i));
	if (seek("SECTORS")) for (int i = 0; i < size; i += sizeof(WADSector)) m_Sectors.push_back(*(WADSector*)(ptr + i));
	if (seek("SIDEDEFS")) for (int i = 0; i < size; i += sizeof(WADSidedef)) m_Sidedefs.push_back(Sidedef(*(WADSidedef*)(ptr + i), m_Sectors));
	if (seek("LINEDEFS")) for (int i = 0; i < size; i += sizeof(WADLinedef)) m_Linedefs.push_back(Linedef(*(WADLinedef*)(ptr + i), m_Sidedefs, m_Vertexes));
	if (seek("SEGS")) for (int i = 0; i < size; i += sizeof(WADSeg)) m_pSegs.push_back(*(WADSeg*)(ptr + i));
	if (seek("THINGS")) for (int i = 0; i < size; i += sizeof(Thing)) m_pThings->AddThing(*(Thing*)(ptr + i));
	if (seek("NODES")) for (int i = 0; i < size; i += sizeof(Node)) m_Nodes.push_back(*(Node*)(ptr + i));
	if (seek("SSECTORS")) for (int i = 0; i < size; i += sizeof(Subsector)) m_Subsector.push_back(*(Subsector*)(ptr + i));

	BuildSeg();
}


void Map::BuildSeg()
{
    WADSeg wadseg;
    Seg seg;

    for (int i = 0; i < m_pSegs.size(); ++i)
    {
		wadseg = m_pSegs[i];

        seg.pStartVertex = m_Vertexes[wadseg.StartVertexID];
        seg.pEndVertex = m_Vertexes[wadseg.EndVertexID];
        // 8.38190317e-8 is to convert from Binary angles (BAMS) to float
        seg.SlopeAngle = ((float)(wadseg.SlopeAngle << 16) * 8.38190317e-8);
        seg.pLinedef = &m_Linedefs[wadseg.LinedefID];
        seg.Direction = wadseg.Direction;
        seg.Offset = (float)(wadseg.Offset << 16) / (float)(1 << 16);

        const Sidedef *pRightSidedef;
        const Sidedef *pLeftSidedef;

        if (seg.Direction)
        {
            pRightSidedef = seg.pLinedef->pLeftSidedef;
            pLeftSidedef = seg.pLinedef->pRightSidedef;
        }
        else
        {
            pRightSidedef = seg.pLinedef->pRightSidedef;
            pLeftSidedef = seg.pLinedef->pLeftSidedef;
        }

        if (pRightSidedef)
        {
            seg.pRightSector = pRightSidedef->pSector;
        }
        else
        {
            seg.pRightSector = nullptr;
        }

        if (pLeftSidedef)
        {
            seg.pLeftSector = pLeftSidedef->pSector;
        }
        else
        {
            seg.pLeftSector = nullptr;
        }

        m_Segs.push_back(seg);
    }
	m_pSegs.clear();
}

void Map::RenderBSPNodes(int iNodeID)
{
    // Masking all the bits exipt the last one
    // to check if this is a subsector
    if (iNodeID & SUBSECTORIDENTIFIER)
    {
        RenderSubsector(iNodeID & (~SUBSECTORIDENTIFIER));
        return;
    }

    bool isOnLeft = IsPointOnLeftSide(m_pPlayer->GetXPosition(), m_pPlayer->GetYPosition(), iNodeID);

    if (isOnLeft)
    {
        RenderBSPNodes(m_Nodes[iNodeID].LeftChildID);
        RenderBSPNodes(m_Nodes[iNodeID].RightChildID);
    }
    else
    {
        RenderBSPNodes(m_Nodes[iNodeID].RightChildID);
        RenderBSPNodes(m_Nodes[iNodeID].LeftChildID);
    }
}

void Map::RenderSubsector(int iSubsectorID)
{
    Subsector &subsector = m_Subsector[iSubsectorID];

    for (int i = 0; i < subsector.SegCount; i++)
    {
        Seg &seg = m_Segs[subsector.FirstSegID + i];
        Angle V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer;
        if (m_pPlayer->ClipVertexesInFOV((seg.pStartVertex), (seg.pEndVertex), V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer))
        {
            m_pViewRenderer->AddWallInFOV(seg, V1Angle, V2Angle, V1AngleFromPlayer, V2AngleFromPlayer);
        }
    }
}

bool Map::IsPointOnLeftSide(int XPosition, int YPosition, int iNodeID)
{
    int dx = XPosition - m_Nodes[iNodeID].XPartition;
    int dy = YPosition - m_Nodes[iNodeID].YPartition;

    return (((dx * m_Nodes[iNodeID].ChangeYPartition) - (dy * m_Nodes[iNodeID].ChangeXPartition)) <= 0);
}

int Map::GetPlayerSubSectorHieght()
{
    int iSubsectorID = (int)(m_Nodes.size() - 1);
    while (!(iSubsectorID & SUBSECTORIDENTIFIER))
    {

        bool isOnLeft = IsPointOnLeftSide(m_pPlayer->GetXPosition(), m_pPlayer->GetYPosition(), iSubsectorID);

        if (isOnLeft)
        {
            iSubsectorID = m_Nodes[iSubsectorID].LeftChildID;
        }
        else
        {
            iSubsectorID = m_Nodes[iSubsectorID].RightChildID;
        }
    }
    Subsector &subsector = m_Subsector[iSubsectorID & (~SUBSECTORIDENTIFIER)];
    Seg &seg = m_Segs[subsector.FirstSegID];
    return seg.pRightSector->FloorHeight;
    
}
