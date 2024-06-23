#pragma once

#include <cstdint>
#include <vector>

enum { kUpperTextureUnpeg = 8, kLowerTextureUnpeg = 16 };

class Patch;
class Texture;
class Flat;
struct Vertex { int16_t x, y; };
struct Sector { int16_t floorHeight, ceilingHeight; const Flat *floortexture, *ceilingtexture; uint16_t lightlevel, type, tag; const Patch *sky; std::vector<const struct Linedef*> linedefs;};
struct Sidedef { int16_t dx, dy; const Texture *uppertexture, *middletexture, *lowertexture; const Sector *sector; };
struct Linedef { Vertex start, end; uint16_t flags, type, sectorTag; const Sidedef *rSidedef, *lSidedef; };
struct Seg { Vertex start, end; float slopeAngle; const Linedef *linedef; uint16_t direction; float offset, len; const Sector *rSector, *lSector; }; // Direction: 0 same as linedef, 1 opposite of linedef. Offset: distance along linedef to start of seg.
struct Viewpoint { int16_t x, y, z; float angle, cosa, sina, pitch; };
struct Thing { int16_t x, y; uint16_t angle, type, flags; };
