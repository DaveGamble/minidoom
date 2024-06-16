#pragma once

#include <list>
#include <map>
#include "Angle.hpp"
#include "DataTypes.hpp"

class ViewRenderer
{
public:
    ViewRenderer(class Map *map, class Player *player, int renderXSize, int renderYSize);
	~ViewRenderer() {}
    void render(uint8_t *pScreenBuffer, int iBufferPitch);
    void addWallInFOV(Seg &seg);

protected:
    struct SolidSegmentRange { int XStart, XEnd; };

    void storeWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle);
	class Map *map;
	class Player *player;
    int renderWidth, renderHeight, halfRenderWidth, halfRenderHeight, distancePlayerToScreen;

    std::list<SolidSegmentRange> solidWallRanges;
    std::vector<int> floorClipHeight;
    std::vector<int> ceilingClipHeight;
    std::map<const Texture*, uint8_t> wallColor;
    std::map<int, Angle> screenXToAngle;
	uint8_t *screenBuffer;
	int rowlen;
};
