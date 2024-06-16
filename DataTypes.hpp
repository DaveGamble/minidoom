#pragma once

#include <cstdint>
#include <vector>

#define SUBSECTORIDENTIFIER 0x8000 // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

// enum ELINEDEFFLAGS { eBLOCKING = 0, eBLOCKMONSTERS = 1, eTWOSIDED = 2, eDONTPEGTOP = 4, eDONTPEGBOTTOM = 8, eSECRET = 16, eSOUNDBLOCK = 32, eDONTDRAW = 64, eDRAW = 128 }; // Unused for now

class Texture;
struct Vertex { int16_t x, y; };
struct Sector { int16_t floorHeight, ceilingHeight; const Texture *floortexture, *ceilingtexture; uint16_t lightlevel, type, tag; };
struct Sidedef { int16_t dx, dy; const Texture *uppertexture, *middletexture, *lowertexture; const Sector *sector; };
struct Linedef { Vertex start, end; uint16_t flags, type, sectorTag; const Sidedef *rSidedef, *lSidedef; };
struct Seg { Vertex start, end; float slopeAngle; const Linedef *linedef; uint16_t direction; float offset; const Sector *rSector, *lSector; }; // Direction: 0 same as linedef, 1 opposite of linedef. Offset: distance along linedef to start of seg.
struct Subsector { uint16_t numSegs, firstSeg; };
struct BBox { int16_t top, bottom, left, right; };
struct Node { int16_t x, y, dx, dy; BBox rBox, lBox; uint16_t rChild, lChild; };

struct Thing { int16_t x, y; uint16_t angle, type, flags; };
class Things
{
public:
	void Add(Thing &thing) {things.push_back(thing);}
	Thing* GetID(int id) { for (auto& t : things) if (t.type == id) return &t; return nullptr; }
protected:
	std::vector<Thing> things;
};
