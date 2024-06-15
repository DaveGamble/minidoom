#pragma once

#include <cstdint>
#include <string.h>
#include "DataTypes.h"

class WADReader
{
public:
	WADReader() {}
	~WADReader() {}

	void ReadHeaderData(const uint8_t *pWADData, int offset, Header &header) { memcpy(&header, pWADData + offset, sizeof(header)); }
	void ReadDirectoryData(const uint8_t *pWADData, int offset, Directory &directory) { memcpy(&directory, pWADData + offset, 16); }
	void ReadVertexData(const uint8_t *pWADData, int offset, Vertex &vertex) { memcpy(&vertex, pWADData + offset, sizeof(vertex)); }
	void ReadSectorData(const uint8_t *pWADData, int offset, WADSector &sector) { memcpy(&sector, pWADData + offset, sizeof(sector)); }
	void ReadSidedefData(const uint8_t *pWADData, int offset, WADSidedef &sidedef) { memcpy(&sidedef, pWADData + offset, sizeof(sidedef)); }
	void ReadLinedefData(const uint8_t *pWADData, int offset, WADLinedef &linedef) { memcpy(&linedef, pWADData + offset, sizeof(linedef)); }
	void ReadThingData(const uint8_t *pWADData, int offset, Thing &thing) { memcpy(&thing, pWADData + offset, sizeof(thing)); }
	void ReadNodeData(const uint8_t *pWADData, int offset, Node &node) { memcpy(&node, pWADData + offset, sizeof(node)); }
	void ReadSubsectorData(const uint8_t *pWADData, int offset, Subsector &subsector) { memcpy(&subsector, pWADData + offset, sizeof(subsector)); }
	void ReadSegData(const uint8_t *pWADData, int offset, WADSeg &seg) { memcpy(&seg, pWADData + offset, sizeof(seg)); }
	void ReadPalette(const uint8_t *pWADData, int offset, WADPalette &palette) { memcpy(&palette, pWADData + offset, sizeof(palette)); }
    void ReadPatchHeader(const uint8_t *pWADData, int offset, WADPatchHeader &patchheader)
	{
		memcpy(&patchheader, pWADData + offset, 8);
		patchheader.pColumnOffsets = new uint32_t[patchheader.Width];
		memcpy(patchheader.pColumnOffsets, pWADData + offset + 8, patchheader.Width * sizeof(uint32_t));
	}

    void ReadPName(const uint8_t *pWADData, int offset, WADPNames &PNames)
	{
		memcpy(&PNames, pWADData + offset, sizeof(uint32_t));
		PNames.PNameOffset = offset + 4;
	}

    void ReadTextureHeader(const uint8_t *pWADData, int offset, WADTextureHeader &textureheader)
	{
		memcpy(&textureheader, pWADData + offset, 8);
		textureheader.pTexturesDataOffset = new uint32_t[textureheader.TexturesCount];
		memcpy(textureheader.pTexturesDataOffset, pWADData + offset + 4, textureheader.TexturesCount * sizeof(uint32_t));	// <-- How is this +4 correct???
	}
    void ReadTextureData(const uint8_t *pWADData, int offset, WADTextureData &texture);
	void ReadTexturePatch(const uint8_t *pWADData, int offset, WADTexturePatch &texturepatch) { memcpy(&texturepatch, pWADData + offset, sizeof(texturepatch)); }
	void Read8Characters(const uint8_t *pWADData, int offset, char *pName) { memcpy(pName, pWADData + offset, 8); }

    int ReadPatchColumn(const uint8_t *pWADData, int offset, PatchColumnData &patch);
    
protected:
	uint16_t Read2Bytes(const uint8_t *pWADData, int offset) { uint16_t ReadValue; memcpy(&ReadValue, pWADData + offset, sizeof(uint16_t)); return ReadValue; }
	uint32_t Read4Bytes(const uint8_t *pWADData, int offset) { uint32_t ReadValue; memcpy(&ReadValue, pWADData + offset, sizeof(uint32_t)); return ReadValue; }
};
