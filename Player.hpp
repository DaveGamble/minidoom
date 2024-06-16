#pragma once

#include "Angle.hpp"
#include "WADLoader.hpp"

class Player
{
public:
	Player(int _id = 1) : id(_id) {}
	~Player() {}

    void Init(Thing *thing, WADLoader *wad)
	{
		weapon = wad->GetPatch("PISGA0");
		if (thing)
		{
			x = thing->x;
			y = thing->y;
			a = thing->angle;
		}
	}
    void MoveForward() { x += a.cos() * moveSpeed; y += a.sin() * moveSpeed; }
	void MoveBackward() { x -= a.cos() * moveSpeed; y -= a.sin() * moveSpeed; }
    void MoveLeftward() { x -= a.sin() * moveSpeed; y += a.cos() * moveSpeed; }
	void MoveRightward() { x += a.sin() * moveSpeed; y -= a.cos() * moveSpeed; }
	void RotateBy(float dt) { a += (dt * rotateSpeed); }
	void RotateLeft() { RotateBy(0.1875f); }
	void RotateRight() { RotateBy(-0.1875f); }
	void Fly() { z += 1; }
	void Sink() { z -= 1; }
	void Think(int subsectorHeight) { z = subsectorHeight + 41; }
	void Render(uint8_t *buf, int rowlen) { weapon->Render(buf, rowlen, -weapon->GetXOffset(), -weapon->GetYOffset()); }

	int GetID() const { return id; }
	int GetXPosition() const { return x; }
	int GetYPosition() const { return y; }
	int GetZPosition() const { return z; }
	Angle GetAngle() const { return a; }

    // Calulate the distance between the player an the vertex.
	float DistanceToPoint(const Vertex &V) const { return sqrt((x - V.x) * (x - V.x) + (y - V.y) * (y - V.y)); }
protected:
	static constexpr int moveSpeed = 4, rotateSpeed = 4;
	int id, x, y, z {41};
	Angle a;
	Patch *weapon;
};
