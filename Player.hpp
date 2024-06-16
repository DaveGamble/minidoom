#pragma once

#include "Angle.hpp"
#include "DataTypes.hpp"
#include "AssetsManager.hpp"
#include "ViewRenderer.h"
#include "Patch.h"

class Player
{
public:
	Player(int iID = 1) : m_iPlayerID(iID) {}
	~Player() {}

    void Init(Thing *thing, AssetsManager *assets)
	{
		m_pWeapon = assets->GetPatch("PISGA0");
		if (thing)
		{
			m_XPosition = thing->XPosition;
			m_YPosition = thing->YPosition;
			m_Angle = thing->Angle;
		}
	}
    void MoveForward() { m_XPosition += m_Angle.GetCosValue() * m_iMoveSpeed; m_YPosition += m_Angle.GetSinValue() * m_iMoveSpeed; }
    void MoveLeftward() { m_XPosition -= m_Angle.GetCosValue() * m_iMoveSpeed; m_YPosition -= m_Angle.GetSinValue() * m_iMoveSpeed; }
	void RotateLeft() { m_Angle += (0.1875f * m_iRotationSpeed); }
	void RotateRight() { m_Angle -= (0.1875f * m_iRotationSpeed); }
	void Fly() { m_ZPosition += 1; }
	void Sink() { m_ZPosition -= 1; }
	void Think(int iSubSectorHieght) { m_ZPosition = iSubSectorHieght + m_EyeLevel; }
	void Render(uint8_t *pScreenBuffer, int iBufferPitch) { m_pWeapon->Render(pScreenBuffer, iBufferPitch, -m_pWeapon->GetXOffset(), -m_pWeapon->GetYOffset()); }

	int GetID() const { return m_iPlayerID; }
	int GetXPosition() const { return m_XPosition; }
	int GetYPosition() const { return m_YPosition; }
	int GetZPosition() const { return m_ZPosition; }
	int GetFOV() { return m_FOV; }

    bool ClipVertexesInFOV(Vertex &V1, Vertex &V2, Angle &V1Angle, Angle &V2Angle, Angle &V1AngleFromPlayer, Angle &V2AngleFromPlayer)
	{
		V1Angle = AngleToVertex(V1);
		V2Angle = AngleToVertex(V2);

		Angle V1ToV2Span = V1Angle - V2Angle;

		if (V1ToV2Span >= 180) return false;
		V1AngleFromPlayer = V1Angle - m_Angle; // Rotate every thing.
		V2AngleFromPlayer = V2Angle - m_Angle;
		
		Angle V1Moved = V1AngleFromPlayer + m_HalfFOV; // Validate and Clip V1. shift angles to be between 0 and 90 (now virtualy we shifted FOV to be in that range)

		if (V1Moved > m_FOV)
		{
			if (V1Moved - m_FOV >= V1ToV2Span) return false; // now we know that V1, is outside the left side of the FOV But we need to check is Also V2 is outside. Lets find out what is the size of the angle outside the FOV // Are both V1 and V2 outside?
			V1AngleFromPlayer = m_HalfFOV; // At this point V2 or part of the line should be in the FOV. We need to clip the V1
		}
		if (m_HalfFOV - V2AngleFromPlayer > m_FOV) V2AngleFromPlayer = -m_HalfFOV; // Validate and Clip V2 // Is V2 outside the FOV?

		V1AngleFromPlayer += 90;
		V2AngleFromPlayer += 90;

		return true;
	}

    // Calulate the distance between the player an the vertex.
	float DistanceToPoint(Vertex &V) { return sqrt((m_XPosition - V.XPosition) * (m_XPosition - V.XPosition) + (m_YPosition - V.YPosition) * (m_YPosition - V.YPosition)); }

	Angle AngleToVertex(Vertex &vertex) { return Angle(atan2f(vertex.YPosition - m_YPosition, vertex.XPosition - m_XPosition) * 180.0f / PI); }
	Angle GetAngle() const { return m_Angle; }

protected:
	int m_iPlayerID, m_XPosition, m_YPosition, m_ZPosition {41}, m_EyeLevel {41}, m_FOV {90}, m_iRotationSpeed {4}, m_iMoveSpeed {4};
	Angle m_Angle, m_HalfFOV {45};
	Patch *m_pWeapon;
};
