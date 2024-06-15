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
    void ReadTextureData(const uint8_t *pWADData, WADTextureData &texture)
	{
		memcpy(texture.TextureName, pWADData, 8);
		texture.TextureName[8] = '\0';
		memcpy(&texture.Flags, pWADData + 8, 14);
		texture.pTexturePatch = new WADTexturePatch[texture.PatchCount];
		memcpy(texture.pTexturePatch, pWADData + 22, texture.PatchCount * 10);
	}
	void Read8Characters(const uint8_t *pWADData, char *pName) { memcpy(pName, pWADData, 8); }

    int ReadPatchColumn(const uint8_t *pWADData, int offset, PatchColumnData &patch)
	{
		patch.TopDelta = pWADData[offset++];
		if (patch.TopDelta == 0xFF) return offset;
		patch.Length = pWADData[offset++];
		patch.PaddingPre = pWADData[offset++];
		// TODO: use smart pointer
		patch.pColumnData = new uint8_t[patch.Length];
		memcpy(patch.pColumnData, pWADData + offset, patch.Length);
		offset += patch.Length;
		patch.PaddingPost = pWADData[offset++];
		return offset;
	}

};
