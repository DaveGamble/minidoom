#pragma once

#include <list>
#include <map>
#include "Angle.hpp"
#include "DataTypes.hpp"

class ViewRenderer
{
public:
    ViewRenderer(class Map *pMap, class Player *pPlayer, int renderXSize, int renderYSize);
	~ViewRenderer() {}
    void Render(uint8_t *pScreenBuffer, int iBufferPitch);
    void AddWallInFOV(Seg &seg, Angle V1Angle, Angle V2Angle, Angle V1AngleFromPlayer, Angle V2AngleFromPlayer);

protected:
    struct SolidSegmentRange
    {
        int XStart;
        int XEnd;
    };

    struct SegmentRenderData
    {
		int V1XScreen;
		int V2XScreen;
		Angle V1Angle;
		Angle V2Angle;
        float DistanceToV1;
        float DistanceToNormal;
        float V1ScaleFactor;
        float V2ScaleFactor;
        float Steps;

        float RightSectorCeiling;
        float RightSectorFloor;
        float CeilingStep;
        float CeilingEnd;
        float FloorStep;
        float FloorStart;

        float LeftSectorCeiling;
        float LeftSectorFloor;

        bool bDrawUpperSection;
        bool bDrawLowerSection;

        float UpperHeightStep;
        float iUpperHeight;
        float LowerHeightStep;
        float iLowerHeight;

        bool UpdateFloor;
        bool UpdateCeiling;

		Seg *pSeg;
    };

    void StoreWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle);

	class Map *m_pMap;
	class Player *m_pPlayer;
    int m_iRenderXSize, m_iRenderYSize, m_HalfScreenWidth, m_HalfScreenHeight, m_iDistancePlayerToScreen;

    std::list<SolidSegmentRange> m_SolidWallRanges;
    std::vector<int> m_FloorClipHeight;
    std::vector<int> m_CeilingClipHeight;
    std::map<const Texture*, uint8_t> m_WallColor;
    std::map<int, Angle> m_ScreenXToAngle;

	uint8_t *m_pScreenBuffer;
	int m_iBufferPitch;

};
