#pragma once

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
			a = thing->angle * M_PI / 65536;
		}
	}
    void moveForward() { x += cos(a) * moveSpeed; y += sin(a) * moveSpeed; }
	void moveBackward() { x -= cos(a) * moveSpeed; y -= sin(a) * moveSpeed; }
    void strafeLeft() { x -= sin(a) * moveSpeed; y += cos(a) * moveSpeed; }
	void strafeRight() { x += sin(a) * moveSpeed; y -= cos(a) * moveSpeed; }
	void rotateBy(float dt) { a += (dt * rotateSpeed); a -= M_PI * 2 * floorf(0.5 * a * M_1_PI); }
	void rotateLeft() { rotateBy(0.1875f); }
	void rotateRight() { rotateBy(-0.1875f); }
	void fly() { z += 1; }
	void sink() { z -= 1; }
	void think(int subsectorHeight) { z = subsectorHeight + 41; }
	void render(uint8_t *buf, int rowlen) { weapon->render(buf, rowlen, -weapon->getXOffset() * 3, -weapon->getYOffset() * 3, 3); }

	int getID() const { return id; }
	int getX() const { return x; }
	int getY() const { return y; }
	int getZ() const { return z; }
	float getAngle() const { return a * 180 * M_1_PI; }
protected:
	static constexpr float moveSpeed = 4, rotateSpeed = 0.06981317008;
	int id, x {0}, y {0}, z {41};
	float a {0};
	Patch *weapon {nullptr};
};
