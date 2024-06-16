#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include <SDL.h>

#include "DataTypes.hpp"
#include "Angle.hpp"

class Map;
class Player;

class ViewRenderer
{
public:
    ViewRenderer(Map *pMap, Player *pPlayer, int renderXSize, int renderYSize);
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

	Map *m_pMap;
	Player *m_pPlayer;
    int m_iRenderXSize, m_iRenderYSize, m_HalfScreenWidth, m_HalfScreenHeight, m_iDistancePlayerToScreen;

    SDL_Color GetWallColor(std::string textureName);


    std::list<SolidSegmentRange> m_SolidWallRanges;
    std::vector<int> m_FloorClipHeight;
    std::vector<int> m_CeilingClipHeight;
    std::map<const Texture*, uint8_t> m_WallColor;
    std::map<int, Angle> m_ScreenXToAngle;

	uint8_t *m_pScreenBuffer;
	int m_iBufferPitch;

};
