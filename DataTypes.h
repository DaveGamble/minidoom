#pragma once

#include <cstdint>
#include "Angle.hpp"

#define SUBSECTORIDENTIFIER 0x8000 // Subsector Identifier is the 16th bit which indicate if the node ID is a subsector. The node ID is stored as uint16 0x8000

// enum ELINEDEFFLAGS { eBLOCKING = 0, eBLOCKMONSTERS = 1, eTWOSIDED = 2, eDONTPEGTOP = 4, eDONTPEGBOTTOM = 8, eSECRET = 16, eSOUNDBLOCK = 32, eDONTDRAW = 64, eDRAW = 128 }; // Unused for now

struct Directory { uint32_t LumpOffset, LumpSize; char LumpName[9] {}; };
struct Thing { int16_t XPosition, YPosition; uint16_t Angle, Type, Flags; };
struct Vertex { int16_t XPosition, YPosition; };
struct WADSector { int16_t FloorHeight, CeilingHeight; char FloorTexture[8], CeilingTexture[8]; uint16_t Lightlevel, Type, Tag; };
struct Sector { int16_t FloorHeight, CeilingHeight; char FloorTexture[9], CeilingTexture[9]; uint16_t Lightlevel, Type, Tag; };
struct WADSidedef { int16_t XOffset, YOffset; char UpperTexture[8], LowerTexture[8], MiddleTexture[8]; uint16_t SectorID; };
struct Sidedef { int16_t XOffset, YOffset; char UpperTexture[9], LowerTexture[9], MiddleTexture[9]; Sector *pSector; };
struct WADLinedef { uint16_t StartVertexID, EndVertexID, Flags, LineType, SectorTag, RightSidedef, LeftSidedef; }; // Sidedef 0xFFFF means there is no sidedef
struct Linedef { Vertex *pStartVertex, *pEndVertex; uint16_t Flags, LineType, SectorTag; Sidedef *pRightSidedef, *pLeftSidedef; };
struct WADSeg { uint16_t StartVertexID, EndVertexID, SlopeAngle, LinedefID, Direction, Offset; }; // Direction: 0 same as linedef, 1 opposite of linedef Offset: distance along linedef to start of seg
struct Seg { Vertex *pStartVertex, *pEndVertex; Angle SlopeAngle; Linedef *pLinedef; uint16_t Direction, Offset; Sector *pRightSector, *pLeftSector; }; // Direction: 0 same as linedef, 1 opposite of linedef. Offset: distance along linedef to start of seg.
struct Subsector { uint16_t SegCount, FirstSegID; };
struct Node {
    int16_t XPartition, YPartition, ChangeXPartition, ChangeYPartition, RightBoxTop, RightBoxBottom, RightBoxLeft, RightBoxRight, LeftBoxTop, LeftBoxBottom, LeftBoxLeft, LeftBoxRight;
    uint16_t RightChildID, LeftChildID;
};
struct WADPatchHeader { uint16_t Width, Height; int16_t LeftOffset, TopOffset; uint32_t *pColumnOffsets; };
struct PatchColumnData { uint8_t TopDelta, Length, PaddingPre, *pColumnData, PaddingPost; };
struct WADPNames { uint32_t PNameCount, PNameOffset; };
struct WADTextureHeader { uint32_t TexturesCount, TexturesOffset, *pTexturesDataOffset; };
struct WADTexturePatch { int16_t XOffset, YOffset; uint16_t PNameIndex, StepDir, ColorMap; }; // StepDir, ColorMap Unused values.
struct WADTextureData { char TextureName[9]; uint32_t Flags; uint16_t Width, Height; uint32_t ColumnDirectory; uint16_t PatchCount; WADTexturePatch *pTexturePatch; };// ColumnDirectory Unused value.
struct WADPalette { uint8_t bytes[768]; };
