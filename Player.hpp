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
	void MoveBackward() { m_XPosition -= m_Angle.GetCosValue() * m_iMoveSpeed; m_YPosition -= m_Angle.GetSinValue() * m_iMoveSpeed; }
    void MoveLeftward() { m_XPosition -= m_Angle.GetSinValue() * m_iMoveSpeed; m_YPosition += m_Angle.GetCosValue() * m_iMoveSpeed; }
	void MoveRightward() { m_XPosition += m_Angle.GetSinValue() * m_iMoveSpeed; m_YPosition -= m_Angle.GetCosValue() * m_iMoveSpeed; }
	void RotateBy(float dt) { m_Angle += (dt * m_iRotationSpeed); }
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
	Angle GetAngle() const { return m_Angle; }

    // Calulate the distance between the player an the vertex.
	float DistanceToPoint(const Vertex &V) const { return sqrt((m_XPosition - V.XPosition) * (m_XPosition - V.XPosition) + (m_YPosition - V.YPosition) * (m_YPosition - V.YPosition)); }
protected:
	int m_iPlayerID, m_XPosition, m_YPosition, m_ZPosition {41}, m_EyeLevel {41}, m_iRotationSpeed {4}, m_iMoveSpeed {4};
	Angle m_Angle;
	Patch *m_pWeapon;
};
