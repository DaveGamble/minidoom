#pragma once

#include <cstdint>
#include <vector>

#define SUBSECTORIDENTIFIER 0x8000 // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

// enum ELINEDEFFLAGS { eBLOCKING = 0, eBLOCKMONSTERS = 1, eTWOSIDED = 2, eDONTPEGTOP = 4, eDONTPEGBOTTOM = 8, eSECRET = 16, eSOUNDBLOCK = 32, eDONTDRAW = 64, eDRAW = 128 }; // Unused for now

struct Thing { int16_t x, y; uint16_t angle, type, flags; };
struct WADTexturePatch { int16_t dx, dy; uint16_t pnameIndex, stepDir, colorMap; }; // StepDir, ColorMap Unused values.
struct WADTextureData { char textureName[8]; uint32_t flags; uint16_t width, height; uint32_t columnDirectory; uint16_t patchCount; WADTexturePatch *texturePatch; };// ColumnDirectory Unused value.

class Texture;
class WADLoader;
struct Vertex { int16_t x, y; };
struct Sector {
	int16_t FloorHeight, CeilingHeight; const Texture *floortexture, *ceilingtexture; uint16_t Lightlevel, Type, Tag;
};
struct Sidedef {
	int16_t XOffset, YOffset; const Texture *uppertexture, *middletexture, *lowertexture; const Sector *pSector;
};
struct Linedef {
	Vertex pStartVertex, pEndVertex; uint16_t Flags, LineType, SectorTag; const Sidedef *pRightSidedef, *pLeftSidedef;
};
struct Seg {
	Vertex start, end; float SlopeAngle; const Linedef *pLinedef; uint16_t Direction; float Offset; const Sector *pRightSector, *pLeftSector; // Direction: 0 same as linedef, 1 opposite of linedef. Offset: distance along linedef to start of seg.
};
struct Subsector { uint16_t SegCount, FirstSegID; };
struct Node { int16_t x, y, dx, dy, RightBoxTop, RightBoxBottom, RightBoxLeft, RightBoxRight, LeftBoxTop, LeftBoxBottom, LeftBoxLeft, LeftBoxRight; uint16_t RightChildID, LeftChildID; };

class Things
{
public:
	void AddThing(Thing &thing) {m_Things.push_back(thing);}
	Thing* GetThingByID(int iID) { for (auto& t : m_Things) if (t.type == iID) return &t; return nullptr; }
protected:
	std::vector<Thing> m_Things;
};
