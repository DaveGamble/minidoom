#include "DataTypes.hpp"
#include "WADLoader.hpp"

Sector::Sector(const WADSector& wadsector, WADLoader *wad)
{
	char name[9] {};
	FloorHeight = wadsector.FloorHeight;
	CeilingHeight = wadsector.CeilingHeight;
	memcpy(name, wadsector.FloorTexture, 8);	floortexture = wad->GetTexture(name);
	memcpy(name, wadsector.CeilingTexture, 8);	ceilingtexture = wad->GetTexture(name);
	Lightlevel = wadsector.Lightlevel;
	Type = wadsector.Type;
	Tag = wadsector.Tag;
}

Sidedef::Sidedef(const WADSidedef& wadsidedef, const std::vector<Sector> &sectors, WADLoader *wad)
{
	char name[9] {};
	XOffset = wadsidedef.XOffset;
	YOffset = wadsidedef.YOffset;
	memcpy(name, wadsidedef.UpperTexture, 8); uppertexture = wad->GetTexture(name);
	memcpy(name, wadsidedef.LowerTexture, 8); lowertexture = wad->GetTexture(name);
	memcpy(name, wadsidedef.MiddleTexture, 8); middletexture = wad->GetTexture(name);
	pSector = &sectors[wadsidedef.SectorID];
}

Linedef::Linedef(const WADLinedef &wadlinedef, const std::vector<Sidedef> &sidedefs, const std::vector<Vertex> &vertices)
{
	pStartVertex = vertices[wadlinedef.StartVertexID];
	pEndVertex = vertices[wadlinedef.EndVertexID];
	Flags = wadlinedef.Flags;
	LineType = wadlinedef.LineType;
	SectorTag = wadlinedef.SectorTag;
	pRightSidedef = (wadlinedef.RightSidedef == 0xFFFF) ? nullptr : &sidedefs[wadlinedef.RightSidedef];
	pLeftSidedef = (wadlinedef.LeftSidedef == 0xFFFF) ? nullptr : &sidedefs[wadlinedef.LeftSidedef];
}

Seg::Seg(const WADSeg &wadseg, const std::vector<Linedef>& linedefs, const std::vector<Vertex> &vertices)
{
	start = vertices[wadseg.StartVertexID];
	end = vertices[wadseg.EndVertexID];
	SlopeAngle = ((float)(wadseg.SlopeAngle << 16) * 8.38190317e-8);	// 8.38190317e-8 is to convert from Binary angles (BAMS) to float
	pLinedef = &linedefs[wadseg.LinedefID];
	Direction = wadseg.Direction;
	Offset = (float)(wadseg.Offset << 16) / (float)(1 << 16);

	const Sidedef *pRightSidedef;
	const Sidedef *pLeftSidedef;
	if (Direction)
	{
		pRightSidedef = pLinedef->pLeftSidedef;
		pLeftSidedef = pLinedef->pRightSidedef;
	}
	else
	{
		pRightSidedef = pLinedef->pRightSidedef;
		pLeftSidedef = pLinedef->pLeftSidedef;
	}
	pRightSector = (pRightSidedef) ? pRightSidedef->pSector : nullptr;
	pLeftSector = (pLeftSidedef) ? pLeftSidedef->pSector : nullptr;
}
