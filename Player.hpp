#pragma once

#include "Angle.hpp"
#include "WADLoader.hpp"

class Player
{
public:
	Player(int _id = 1) : id(_id) {}
	~Player() {}

    void init(Thing *thing, WADLoader *wad)
	{
		weapon = wad->getPatch("PISGA0");
		if (thing)
		{
			x = thing->x;
			y = thing->y;
			a = thing->angle;
		}
	}
    void moveForward() { x += a.cos() * moveSpeed; y += a.sin() * moveSpeed; }
	void moveBackward() { x -= a.cos() * moveSpeed; y -= a.sin() * moveSpeed; }
    void strafeLeft() { x -= a.sin() * moveSpeed; y += a.cos() * moveSpeed; }
	void strafeRight() { x += a.sin() * moveSpeed; y -= a.cos() * moveSpeed; }
	void rotateBy(float dt) { a += (dt * rotateSpeed); }
	void rotateLeft() { rotateBy(0.1875f); }
	void rotateRight() { rotateBy(-0.1875f); }
	void fly() { z += 1; }
	void sink() { z -= 1; }
	void think(int subsectorHeight) { z = subsectorHeight + 41; }
	void render(uint8_t *buf, int rowlen) { weapon->Render(buf, rowlen, -weapon->GetXOffset(), -weapon->GetYOffset()); }

	int getID() const { return id; }
	int getX() const { return x; }
	int getY() const { return y; }
	int getZ() const { return z; }
	Angle getAngle() const { return a; }
protected:
	static constexpr int moveSpeed = 4, rotateSpeed = 4;
	int id, x {0}, y {0}, z {41};
	Angle a {0};
	Patch *weapon {nullptr};
};
