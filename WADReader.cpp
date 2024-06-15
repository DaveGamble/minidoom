#include "WADReader.h"
#include <cstring>


int WADReader::ReadPatchColumn(const uint8_t *pWADData, int offset, PatchColumnData &patch)
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


void WADReader::ReadTextureData(const uint8_t *pWADData, int offset, WADTextureData &texture)
{
	memcpy(texture.TextureName, pWADData + offset, 8);
    texture.TextureName[8] = '\0';

    texture.Flags = Read4Bytes(pWADData, offset + 8);
    texture.Width = Read2Bytes(pWADData, offset + 12);
    texture.Height = Read2Bytes(pWADData, offset + 14);
    texture.ColumnDirectory = Read4Bytes(pWADData, offset + 16);
    texture.PatchCount = Read2Bytes(pWADData, offset + 20);
    texture.pTexturePatch = new WADTexturePatch[texture.PatchCount];

    offset += 22;
    for (int i = 0; i < texture.PatchCount; ++i)
    {
        ReadTexturePatch(pWADData, offset, texture.pTexturePatch[i]);
        offset += 10;
    }
}
