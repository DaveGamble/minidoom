#include "DoomEngine.h"

int main(int argc, char* argv[])
{
    DoomEngine game("DOOM.WAD", "E1M1");
	while (!game.Tick()) {}
    return 0;
}
