#pragma once

#include <cstdint>
#include <string.h>
#include "DataTypes.h"

class WADReader
{
public:
	WADReader() {}
	~WADReader() {}

	void ReadHeaderData(const uint8_t *pWADData, Header &header) { memcpy(&header, pWADData, sizeof(header)); }
	void ReadDirectoryData(const uint8_t *pWADData, Directory &directory) { memcpy(&directory, pWADData, 16); }
	void ReadVertexData(const uint8_t *pWADData, Vertex &vertex) { memcpy(&vertex, pWADData, sizeof(vertex)); }
	void ReadSectorData(const uint8_t *pWADData, WADSector &sector) { memcpy(&sector, pWADData, sizeof(sector)); }
	void ReadSidedefData(const uint8_t *pWADData, WADSidedef &sidedef) { memcpy(&sidedef, pWADData, sizeof(sidedef)); }
	void ReadLinedefData(const uint8_t *pWADData, WADLinedef &linedef) { memcpy(&linedef, pWADData, sizeof(linedef)); }
	void ReadThingData(const uint8_t *pWADData, Thing &thing) { memcpy(&thing, pWADData, sizeof(thing)); }
	void ReadNodeData(const uint8_t *pWADData, Node &node) { memcpy(&node, pWADData, sizeof(node)); }
	void ReadSubsectorData(const uint8_t *pWADData, Subsector &subsector) { memcpy(&subsector, pWADData, sizeof(subsector)); }
	void ReadSegData(const uint8_t *pWADData, WADSeg &seg) { memcpy(&seg, pWADData, sizeof(seg)); }
	void ReadPalette(const uint8_t *pWADData, WADPalette &palette) { memcpy(&palette, pWADData, sizeof(palette)); }
    void ReadPatchHeader(const uint8_t *pWADData, WADPatchHeader &patchheader)
	{
		memcpy(&patchheader, pWADData, 8);
		patchheader.pColumnOffsets = new uint32_t[patchheader.Width];
		memcpy(patchheader.pColumnOffsets, pWADData + 8, patchheader.Width * sizeof(uint32_t));
	}

    void ReadPName(const uint8_t *pWADData, int offset, WADPNames &PNames)
	{
		memcpy(&PNames, pWADData + offset, sizeof(uint32_t));
		PNames.PNameOffset = offset + 4;
	}

    void ReadTextureHeader(const uint8_t *pWADData, WADTextureHeader &textureheader)
	{
		memcpy(&textureheader, pWADData, 8);
		textureheader.pTexturesDataOffset = new uint32_t[textureheader.TexturesCount];
		memcpy(textureheader.pTexturesDataOffset, pWADData + 4, textureheader.TexturesCount * sizeof(uint32_t));	// <-- How is this +4 correct???
	}
    void ReadTextureData(const uint8_t *pWADData, int offset, WADTextureData &texture);
	void ReadTexturePatch(const uint8_t *pWADData, WADTexturePatch &texturepatch) { memcpy(&texturepatch, pWADData, sizeof(texturepatch)); }
	void Read8Characters(const uint8_t *pWADData, char *pName) { memcpy(pName, pWADData, 8); }

    int ReadPatchColumn(const uint8_t *pWADData, int offset, PatchColumnData &patch);
    
protected:
	uint16_t Read2Bytes(const uint8_t *pWADData, int offset) { uint16_t ReadValue; memcpy(&ReadValue, pWADData + offset, sizeof(uint16_t)); return ReadValue; }
	uint32_t Read4Bytes(const uint8_t *pWADData, int offset) { uint32_t ReadValue; memcpy(&ReadValue, pWADData + offset, sizeof(uint32_t)); return ReadValue; }
};
