#include "ViewRenderer.h"

#include <algorithm>
#include "Map.h"
#include "Player.hpp"
#include "Texture.h"

ViewRenderer::ViewRenderer(Map *pMap, Player *pPlayer, int renderXSize, int renderYSize)
: m_pMap(pMap)
, m_pPlayer(pPlayer)
, m_iRenderXSize(renderXSize)
, m_iRenderYSize(renderYSize)
, m_HalfScreenWidth(renderXSize / 2)
, m_HalfScreenHeight(renderYSize / 2)
, m_iDistancePlayerToScreen(m_HalfScreenWidth / tan(90 * 0.5 * M_PI / 180))	// 90 here is FOV
{
	for (int i = 0; i <= m_iRenderXSize; ++i)
		m_ScreenXToAngle[i] = atan((m_HalfScreenWidth - i) / (float)m_iDistancePlayerToScreen) * 180 / M_PI;

	m_CeilingClipHeight.resize(m_iRenderXSize);
	m_FloorClipHeight.resize(m_iRenderXSize);
}


void ViewRenderer::Render(uint8_t *pScreenBuffer, int iBufferPitch)
{
	m_pScreenBuffer = pScreenBuffer;
	m_iBufferPitch = iBufferPitch;
	m_SolidWallRanges.clear();
	m_SolidWallRanges.push_back({INT_MIN, -1});
	m_SolidWallRanges.push_back({m_iRenderXSize, INT_MAX});
	std::fill(m_CeilingClipHeight.begin(), m_CeilingClipHeight.end(), -1);
	std::fill(m_FloorClipHeight.begin(), m_FloorClipHeight.end(), m_iRenderYSize);

	m_pMap->Render3DView();
}

void ViewRenderer::AddWallInFOV(Seg &seg, Angle V1Angle, Angle V2Angle, Angle V1AngleFromPlayer, Angle V2AngleFromPlayer)
{
	auto AngleToScreen = [&](Angle angle) {
		return m_iDistancePlayerToScreen + round(tanf((90 - angle.get()) * M_PI / 180.0f) * m_HalfScreenWidth);
	};

    int V1XScreen = AngleToScreen(V1AngleFromPlayer), V2XScreen = AngleToScreen(V2AngleFromPlayer); // Find Wall X Coordinates
    if (V1XScreen == V2XScreen) return; // Skip same pixel wall
	bool solid = false;
    if (!seg.lSector) solid = true; // Handle solid walls
    else if (seg.lSector->ceilingHeight <= seg.rSector->floorHeight || seg.lSector->floorHeight >= seg.rSector->ceilingHeight) // Handle closed door
        solid = true;
    else if (seg.rSector->ceilingHeight != seg.lSector->ceilingHeight || seg.rSector->floorHeight != seg.lSector->floorHeight) // Windowed walls
        solid = false;
	else return;

	if (solid && m_SolidWallRanges.size() < 2) return;
    SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen }; // Find clip window
    std::list<SolidSegmentRange>::iterator FoundClipWall = m_SolidWallRanges.begin();
    while (FoundClipWall != m_SolidWallRanges.end() && FoundClipWall->XEnd < CurrentWall.XStart - 1) ++FoundClipWall;

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            StoreWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle); //All of the wall is visible, so insert it
            if (solid) m_SolidWallRanges.insert(FoundClipWall, CurrentWall);
            return;
        }
        StoreWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle); // The end is already included, just update start
        if (solid) FoundClipWall->XStart = CurrentWall.XStart;
    }
    
    if (CurrentWall.XEnd <= FoundClipWall->XEnd) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;
    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        StoreWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle); // partialy clipped by other walls, store each fragment
        ++NextWall;
        if (CurrentWall.XEnd <= NextWall->XEnd)
        {
			if (solid)
			{
				FoundClipWall->XEnd = NextWall->XEnd;
				if (NextWall != FoundClipWall) m_SolidWallRanges.erase(++FoundClipWall, ++NextWall);
			}
            return;
        }
    }
    StoreWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
	if (solid)
	{
		FoundClipWall->XEnd = CurrentWall.XEnd;
		if (NextWall != FoundClipWall) m_SolidWallRanges.erase(++FoundClipWall, ++NextWall);
	}
}

void ViewRenderer::StoreWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
	auto GetScaleFactor = [&](int VXScreen, Angle SegToNormalAngle, float DistanceToNormal) {
		Angle SkewAngle = m_ScreenXToAngle[VXScreen] + m_pPlayer->GetAngle() - SegToNormalAngle;
		return std::clamp((m_iDistancePlayerToScreen * SkewAngle.cos()) / (DistanceToNormal * m_ScreenXToAngle[VXScreen].cos()), 0.00390625f, 64.0f);
	};

    // Calculate the distance to the first edge of the wall
    Angle Angle90(90);
	
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
	
    SegmentRenderData RenderData { 0 };
    Angle SegToNormalAngle = Angle(seg.slopeAngle) + Angle90;

    Angle NomalToV1Angle = SegToNormalAngle.get() - V1Angle.get();

    // Normal angle is 90 degree to wall
    Angle SegToPlayerAngle = Angle90 - NomalToV1Angle;

	RenderData.V1XScreen = V1XScreen;
	RenderData.V2XScreen = V2XScreen;
	RenderData.V1Angle = V1Angle;
	RenderData.V2Angle = V2Angle;

    RenderData.DistanceToV1 = m_pPlayer->DistanceToPoint(seg.start);
    RenderData.DistanceToNormal = SegToPlayerAngle.sin() * RenderData.DistanceToV1;

    RenderData.V1ScaleFactor = GetScaleFactor(V1XScreen, SegToNormalAngle, RenderData.DistanceToNormal);
    RenderData.V2ScaleFactor = GetScaleFactor(V2XScreen, SegToNormalAngle, RenderData.DistanceToNormal);

    RenderData.Steps = (RenderData.V2ScaleFactor - RenderData.V1ScaleFactor) / (V2XScreen - V1XScreen);

    RenderData.RightSectorCeiling = seg.rSector->ceilingHeight - m_pPlayer->GetZPosition();
    RenderData.RightSectorFloor = seg.rSector->floorHeight - m_pPlayer->GetZPosition();

    RenderData.CeilingStep = -(RenderData.RightSectorCeiling * RenderData.Steps);
    RenderData.CeilingEnd = round(m_HalfScreenHeight - (RenderData.RightSectorCeiling * RenderData.V1ScaleFactor));

    RenderData.FloorStep = -(RenderData.RightSectorFloor * RenderData.Steps);
    RenderData.FloorStart = round(m_HalfScreenHeight - (RenderData.RightSectorFloor * RenderData.V1ScaleFactor));

	RenderData.pSeg = &seg;

    if (seg.lSector)
    {
        RenderData.LeftSectorCeiling = seg.lSector->ceilingHeight - m_pPlayer->GetZPosition();
        RenderData.LeftSectorFloor = seg.lSector->floorHeight - m_pPlayer->GetZPosition();

		if (!RenderData.pSeg->lSector)
		{
			RenderData.UpdateFloor = true;
			RenderData.UpdateCeiling = true;
			return;
		}

		RenderData.UpdateCeiling = (RenderData.LeftSectorCeiling != RenderData.RightSectorCeiling);
		RenderData.UpdateFloor = (RenderData.LeftSectorFloor != RenderData.RightSectorFloor);

		if (RenderData.pSeg->lSector->ceilingHeight <= RenderData.pSeg->rSector->floorHeight || RenderData.pSeg->lSector->floorHeight >= RenderData.pSeg->rSector->ceilingHeight) // closed door
			RenderData.UpdateCeiling = RenderData.UpdateFloor = true;
		if (RenderData.pSeg->rSector->ceilingHeight <= m_pPlayer->GetZPosition()) // below view plane
			RenderData.UpdateCeiling = false;
		if (RenderData.pSeg->rSector->floorHeight >= m_pPlayer->GetZPosition()) // above view plane
			RenderData.UpdateFloor = false;

        if (RenderData.LeftSectorCeiling < RenderData.RightSectorCeiling)
        {
            RenderData.bDrawUpperSection = true;
            RenderData.UpperHeightStep = -(RenderData.LeftSectorCeiling * RenderData.Steps);
            RenderData.iUpperHeight = round(m_HalfScreenHeight - (RenderData.LeftSectorCeiling * RenderData.V1ScaleFactor));
        }

        if (RenderData.LeftSectorFloor > RenderData.RightSectorFloor)
        {
            RenderData.bDrawLowerSection = true;
            RenderData.LowerHeightStep = -(RenderData.LeftSectorFloor * RenderData.Steps);
            RenderData.iLowerHeight = round(m_HalfScreenHeight - (RenderData.LeftSectorFloor * RenderData.V1ScaleFactor));
        }
    }

	auto GetSectionColor = [&](const Texture *texture) {
		if (!m_WallColor.count(texture)) m_WallColor[texture] = rand() & 255;
		return m_WallColor[texture];
	};
	auto DrawVerticalLine = [&](int iX, int iStartY, int iEndY, uint8_t color) {
		for (; iStartY < iEndY; iStartY++) m_pScreenBuffer[m_iBufferPitch * iStartY + iX] = color;
	};
	
	for (int iXCurrent = RenderData.V1XScreen; iXCurrent <= RenderData.V2XScreen; iXCurrent++)
    {
        int CurrentCeilingEnd = std::max(RenderData.CeilingEnd, m_CeilingClipHeight[iXCurrent] + 1.f);
        int CurrentFloorStart = std::min(RenderData.FloorStart, m_FloorClipHeight[iXCurrent] - 1.f);

		if (CurrentCeilingEnd > CurrentFloorStart)
		{
			RenderData.CeilingEnd += RenderData.CeilingStep;
			RenderData.FloorStart += RenderData.FloorStep;
			continue;
		}

        if (RenderData.pSeg->lSector)
        {
			if (RenderData.bDrawUpperSection)
			{
				int iUpperHeight = std::min(m_FloorClipHeight[iXCurrent] - 1.f, RenderData.iUpperHeight);
				RenderData.iUpperHeight += RenderData.UpperHeightStep;
				if (iUpperHeight >= CurrentCeilingEnd)
					DrawVerticalLine(iXCurrent, CurrentCeilingEnd, iUpperHeight, GetSectionColor(RenderData.pSeg->linedef->rSidedef->uppertexture));
				m_CeilingClipHeight[iXCurrent] = std::max(CurrentCeilingEnd - 1, iUpperHeight);
			}
			else if (RenderData.UpdateCeiling)
				m_CeilingClipHeight[iXCurrent] = CurrentCeilingEnd - 1;

			if (RenderData.bDrawLowerSection)
			{
				int iLowerHeight = std::max(RenderData.iLowerHeight, m_CeilingClipHeight[iXCurrent] + 1.f);
				RenderData.iLowerHeight += RenderData.LowerHeightStep;
				if (iLowerHeight <= CurrentFloorStart)
					DrawVerticalLine(iXCurrent, iLowerHeight, CurrentFloorStart, GetSectionColor(RenderData.pSeg->linedef->rSidedef->lowertexture));
				m_FloorClipHeight[iXCurrent] = std::min(CurrentFloorStart + 1, iLowerHeight);
			}
			else if (RenderData.UpdateFloor)
				m_FloorClipHeight[iXCurrent] = CurrentFloorStart + 1;
		}
        else
		{
			DrawVerticalLine(iXCurrent, CurrentCeilingEnd, CurrentFloorStart, GetSectionColor(RenderData.pSeg->linedef->rSidedef->middletexture));
			m_CeilingClipHeight[iXCurrent] = m_iRenderYSize;
			m_FloorClipHeight[iXCurrent] = -1;

		}
        RenderData.CeilingEnd += RenderData.CeilingStep;
        RenderData.FloorStart += RenderData.FloorStep;
    }
}


