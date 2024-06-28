#pragma once
#include "DataTypes.hpp"

class Flat
{
public:
	Flat(const char *_name, const uint8_t *_data) : name(_name) { memcpy(data, _data, 4096); }
	uint8_t pixel(int u, int v) const { return data[(64 * v + (u & 63)) & 4095]; }
	const char *getName() const {return name;}
protected:
	const char *name;
	uint8_t data[4096];
};
