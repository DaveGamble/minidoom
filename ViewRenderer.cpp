#include "ViewRenderer.h"

#include <math.h>
#include <iostream>
#include <algorithm>

#include "Map.h"
#include "Texture.h"
#include "Player.hpp"
#include "AssetsManager.hpp"

ViewRenderer::ViewRenderer(Map *pMap, Player *pPlayer, int renderXSize, int renderYSize)
: m_pMap(pMap)
, m_pPlayer(pPlayer)
, m_iRenderXSize(renderXSize)
, m_iRenderYSize(renderYSize)
, m_HalfScreenWidth(renderXSize / 2)
, m_HalfScreenHeight(renderYSize / 2)
, m_iDistancePlayerToScreen(m_HalfScreenWidth / tan(90 * 0.5 * PI / 180))	// 90 here is FOV
{
	for (int i = 0; i <= m_iRenderXSize; ++i)
		m_ScreenXToAngle[i] = atan((m_HalfScreenWidth - i) / (float)m_iDistancePlayerToScreen) * 180 / PI;

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
		return m_iDistancePlayerToScreen + round(tanf((90 - angle.GetValue()) * PI / 180.0f) * m_HalfScreenWidth);
	};

    int V1XScreen = AngleToScreen(V1AngleFromPlayer), V2XScreen = AngleToScreen(V2AngleFromPlayer); // Find Wall X Coordinates
    if (V1XScreen == V2XScreen) return; // Skip same pixel wall
    if (!seg.pLeftSector) ClipSolidWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle); // Handle solid walls
    else if (seg.pLeftSector->CeilingHeight <= seg.pRightSector->FloorHeight || seg.pLeftSector->FloorHeight >= seg.pRightSector->CeilingHeight) // Handle closed door
        ClipSolidWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
    else if (seg.pRightSector->CeilingHeight != seg.pLeftSector->CeilingHeight || seg.pRightSector->FloorHeight != seg.pLeftSector->FloorHeight) // Windowed walls
        ClipPassWalls(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
}

void ViewRenderer::ClipSolidWalls(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    if (m_SolidWallRanges.size() < 2) return;
    SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen }; // Find clip window
    std::list<SolidSegmentRange>::iterator FoundClipWall = m_SolidWallRanges.begin();
    while (FoundClipWall != m_SolidWallRanges.end() && FoundClipWall->XEnd < CurrentWall.XStart - 1) ++FoundClipWall;

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            StoreWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle); //All of the wall is visible, so insert it
            m_SolidWallRanges.insert(FoundClipWall, CurrentWall);
            return;
        }
        StoreWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle); // The end is already included, just update start
        FoundClipWall->XStart = CurrentWall.XStart;
    }
    
    if (CurrentWall.XEnd <= FoundClipWall->XEnd) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;
    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        StoreWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle); // partialy clipped by other walls, store each fragment
        ++NextWall;
        if (CurrentWall.XEnd <= NextWall->XEnd)
        {
            FoundClipWall->XEnd = NextWall->XEnd;
            if (NextWall != FoundClipWall) m_SolidWallRanges.erase(++FoundClipWall, ++NextWall);
            return;
        }
    }
    StoreWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
    FoundClipWall->XEnd = CurrentWall.XEnd;
    if (NextWall != FoundClipWall) m_SolidWallRanges.erase(++FoundClipWall, ++NextWall);
}

void ViewRenderer::ClipPassWalls(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    SolidSegmentRange CurrentWall = { V1XScreen, V2XScreen }; // Find clip window
    std::list<SolidSegmentRange>::iterator FoundClipWall = m_SolidWallRanges.begin();
    while (FoundClipWall != m_SolidWallRanges.end() && FoundClipWall->XEnd < CurrentWall.XStart - 1) ++FoundClipWall;

    if (CurrentWall.XStart < FoundClipWall->XStart)
    {
        if (CurrentWall.XEnd < FoundClipWall->XStart - 1)
        {
            StoreWallRange(seg, CurrentWall.XStart, CurrentWall.XEnd, V1Angle, V2Angle); //All of the wall is visible, so insert it
            return;
        }
        StoreWallRange(seg, CurrentWall.XStart, FoundClipWall->XStart - 1, V1Angle, V2Angle); // The end is already included, just update start
    }
    
    if (CurrentWall.XEnd <= FoundClipWall->XEnd) return; // This part is already occupied
    std::list<SolidSegmentRange>::iterator NextWall = FoundClipWall;
    while (CurrentWall.XEnd >= next(NextWall, 1)->XStart - 1)
    {
        StoreWallRange(seg, NextWall->XEnd + 1, next(NextWall, 1)->XStart - 1, V1Angle, V2Angle); // partialy clipped by other walls, store each fragment
        ++NextWall;
        if (CurrentWall.XEnd <= NextWall->XEnd) return;
    }
    StoreWallRange(seg, NextWall->XEnd + 1, CurrentWall.XEnd, V1Angle, V2Angle);
}

void ViewRenderer::StoreWallRange(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    // For now just lets sotre the range 
    CalculateWallHeight(seg, V1XScreen, V2XScreen, V1Angle, V2Angle);
}

void ViewRenderer::CalculateWallHeight(Seg &seg, int V1XScreen, int V2XScreen, Angle V1Angle, Angle V2Angle)
{
    // Calculate the distance to the first edge of the wall
    Angle Angle90(90);
    SegmentRenderData RenderData { 0 };
    Angle SegToNormalAngle = seg.SlopeAngle + Angle90;

    //Angle NomalToV1Angle = abs(SegToNormalAngle.GetSignedValue() - V1Angle.GetSignedValue());
    Angle NomalToV1Angle = SegToNormalAngle.GetValue() - V1Angle.GetValue();

    // Normal angle is 90 degree to wall
    Angle SegToPlayerAngle = Angle90 - NomalToV1Angle;

	RenderData.V1XScreen = V1XScreen;
	RenderData.V2XScreen = V2XScreen;
	RenderData.V1Angle = V1Angle;
	RenderData.V2Angle = V2Angle;

    RenderData.DistanceToV1 = m_pPlayer->DistanceToPoint(seg.pStartVertex);
    RenderData.DistanceToNormal = SegToPlayerAngle.GetSinValue() * RenderData.DistanceToV1;

    RenderData.V1ScaleFactor = GetScaleFactor(V1XScreen, SegToNormalAngle, RenderData.DistanceToNormal);
    RenderData.V2ScaleFactor = GetScaleFactor(V2XScreen, SegToNormalAngle, RenderData.DistanceToNormal);

    RenderData.Steps = (RenderData.V2ScaleFactor - RenderData.V1ScaleFactor) / (V2XScreen - V1XScreen);

    RenderData.RightSectorCeiling = seg.pRightSector->CeilingHeight - m_pPlayer->GetZPosition();
    RenderData.RightSectorFloor = seg.pRightSector->FloorHeight - m_pPlayer->GetZPosition();

    RenderData.CeilingStep = -(RenderData.RightSectorCeiling * RenderData.Steps);
    RenderData.CeilingEnd = round(m_HalfScreenHeight - (RenderData.RightSectorCeiling * RenderData.V1ScaleFactor));

    RenderData.FloorStep = -(RenderData.RightSectorFloor * RenderData.Steps);
    RenderData.FloorStart = round(m_HalfScreenHeight - (RenderData.RightSectorFloor * RenderData.V1ScaleFactor));

	RenderData.pSeg = &seg;

    if (seg.pLeftSector)
    {
        RenderData.LeftSectorCeiling = seg.pLeftSector->CeilingHeight - m_pPlayer->GetZPosition();
        RenderData.LeftSectorFloor = seg.pLeftSector->FloorHeight - m_pPlayer->GetZPosition();

        CeilingFloorUpdate(RenderData);

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

    RenderSegment(RenderData);
}

void ViewRenderer::CeilingFloorUpdate(SegmentRenderData &RenderData)
{
    if (!RenderData.pSeg->pLeftSector)
    {
        RenderData.UpdateFloor = true;
        RenderData.UpdateCeiling = true;
        return;
    }

	RenderData.UpdateCeiling = (RenderData.LeftSectorCeiling != RenderData.RightSectorCeiling);
	RenderData.UpdateFloor = (RenderData.LeftSectorFloor != RenderData.RightSectorFloor);

	if (RenderData.pSeg->pLeftSector->CeilingHeight <= RenderData.pSeg->pRightSector->FloorHeight || RenderData.pSeg->pLeftSector->FloorHeight >= RenderData.pSeg->pRightSector->CeilingHeight) // closed door
        RenderData.UpdateCeiling = RenderData.UpdateFloor = true;
    if (RenderData.pSeg->pRightSector->CeilingHeight <= m_pPlayer->GetZPosition()) // below view plane
        RenderData.UpdateCeiling = false;
    if (RenderData.pSeg->pRightSector->FloorHeight >= m_pPlayer->GetZPosition()) // above view plane
        RenderData.UpdateFloor = false;
}

float ViewRenderer::GetScaleFactor(int VXScreen, Angle SegToNormalAngle, float DistanceToNormal)
{
    Angle SkewAngle = m_ScreenXToAngle[VXScreen] + m_pPlayer->GetAngle() - SegToNormalAngle;
	return std::clamp((m_iDistancePlayerToScreen * SkewAngle.GetCosValue()) / (DistanceToNormal * m_ScreenXToAngle[VXScreen].GetCosValue()), 0.00390625f, 64.0f);
}

void ViewRenderer::CalculateCeilingFloorHeight(Seg &seg, int VXScreen, float DistanceToV, float &CeilingVOnScreen, float &FloorVOnScreen)
{
    float Ceiling = seg.pRightSector->CeilingHeight - m_pPlayer->GetZPosition();
    float Floor = seg.pRightSector->FloorHeight - m_pPlayer->GetZPosition();
    float DistanceToVScreen = m_iDistancePlayerToScreen / (m_ScreenXToAngle[VXScreen].GetCosValue() * DistanceToV);
	CeilingVOnScreen = m_HalfScreenHeight + ((Ceiling > 0) ? -1 : 1) * abs(Ceiling) * DistanceToVScreen;
	FloorVOnScreen = m_HalfScreenHeight + ((Floor > 0) ? -1 : 1) * abs(Floor) * DistanceToVScreen;
}

void ViewRenderer::PartialSeg(Seg &seg, Angle &V1Angle, Angle &V2Angle, float &DistanceToV, bool IsLeftSide)
{
    float SideC = sqrt(pow(seg.pStartVertex.XPosition - seg.pEndVertex.XPosition, 2) + pow(seg.pStartVertex.YPosition - seg.pEndVertex.YPosition, 2));
    Angle V1toV2Span = V1Angle - V2Angle;
    Angle AngleB(asinf(DistanceToV * V1toV2Span.GetSinValue() / SideC) * 180.0 / PI);
    Angle AngleA(180 - V1toV2Span.GetValue() - AngleB.GetValue());
    Angle AngleVToFOV = (IsLeftSide) ? V1Angle - (m_pPlayer->GetAngle() + 45) : (m_pPlayer->GetAngle() - 45) - V2Angle;
    Angle NewAngleB(180 - AngleVToFOV.GetValue() - AngleA.GetValue());
    DistanceToV = DistanceToV * AngleA.GetSinValue() / NewAngleB.GetSinValue();
}

void ViewRenderer::RenderSegment(SegmentRenderData &RenderData)
{
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

        if (RenderData.pSeg->pLeftSector)
        {
            DrawUpperSection(RenderData, iXCurrent, CurrentCeilingEnd);
            DrawLowerSection(RenderData, iXCurrent, CurrentFloorStart);
        }
        else
            DrawMiddleSection(RenderData, iXCurrent, CurrentCeilingEnd, CurrentFloorStart);

        RenderData.CeilingEnd += RenderData.CeilingStep;
        RenderData.FloorStart += RenderData.FloorStep;
    }
}

void ViewRenderer::DrawMiddleSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentCeilingEnd, int CurrentFloorStart)
{
	DrawVerticalLine(iXCurrent, CurrentCeilingEnd, CurrentFloorStart, GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->MiddleTexture));
    m_CeilingClipHeight[iXCurrent] = m_iRenderYSize;
    m_FloorClipHeight[iXCurrent] = -1;
}

void ViewRenderer::DrawLowerSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentFloorStart)
{
    if (RenderData.bDrawLowerSection)
    {
		int iLowerHeight = std::max(RenderData.iLowerHeight, m_CeilingClipHeight[iXCurrent] + 1.f);
        RenderData.iLowerHeight += RenderData.LowerHeightStep;
        if (iLowerHeight <= CurrentFloorStart)
			DrawVerticalLine(iXCurrent, iLowerHeight, CurrentFloorStart, GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->LowerTexture));
		m_FloorClipHeight[iXCurrent] = std::min(CurrentFloorStart + 1, iLowerHeight);
    }
    else if (RenderData.UpdateFloor)
        m_FloorClipHeight[iXCurrent] = CurrentFloorStart + 1;
}

void ViewRenderer::DrawUpperSection(ViewRenderer::SegmentRenderData &RenderData, int iXCurrent, int CurrentCeilingEnd)
{
    if (RenderData.bDrawUpperSection)
    {
		int iUpperHeight = std::min(m_FloorClipHeight[iXCurrent] - 1.f, RenderData.iUpperHeight);
        RenderData.iUpperHeight += RenderData.UpperHeightStep;
        if (iUpperHeight >= CurrentCeilingEnd)
			DrawVerticalLine(iXCurrent, CurrentCeilingEnd, iUpperHeight, GetSectionColor(RenderData.pSeg->pLinedef->pRightSidedef->UpperTexture));
		m_CeilingClipHeight[iXCurrent] = std::max(CurrentCeilingEnd - 1, iUpperHeight);
    }
    else if (RenderData.UpdateCeiling)
        m_CeilingClipHeight[iXCurrent] = CurrentCeilingEnd - 1;
}

uint8_t ViewRenderer::GetSectionColor(const std::string &TextureName)
{
    if (!m_WallColor.count(TextureName)) m_WallColor[TextureName] = rand() & 255;
	return m_WallColor[TextureName];
}

void ViewRenderer::DrawVerticalLine(int iX, int iStartY, int iEndY, uint8_t color)
{
	for (; iStartY < iEndY; iStartY++) m_pScreenBuffer[m_iBufferPitch * iStartY + iX] = color;
}
