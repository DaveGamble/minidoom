#include "DataTypes.hpp"

Sector::Sector(const WADSector& wadsector)
{
	FloorHeight = wadsector.FloorHeight;
	CeilingHeight = wadsector.CeilingHeight;
	strncpy(FloorTexture, wadsector.FloorTexture, 8);
	FloorTexture[8] = '\0';
	strncpy(CeilingTexture, wadsector.CeilingTexture, 8);
	CeilingTexture[8] = '\0';
	Lightlevel = wadsector.Lightlevel;
	Type = wadsector.Type;
	Tag = wadsector.Tag;
}

Sidedef::Sidedef(const WADSidedef& wadsidedef, const std::vector<Sector> &sectors)
{
	XOffset = wadsidedef.XOffset;
	YOffset = wadsidedef.YOffset;
	strncpy(UpperTexture, wadsidedef.UpperTexture, 8);
	UpperTexture[8] = '\0';
	strncpy(LowerTexture, wadsidedef.LowerTexture, 8);
	LowerTexture[8] = '\0';
	strncpy(MiddleTexture, wadsidedef.MiddleTexture, 8);
	MiddleTexture[8] = '\0';
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
