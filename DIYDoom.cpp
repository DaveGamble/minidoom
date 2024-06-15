#include "DoomEngine.h"

int main(int argc, char* argv[])
{
    DoomEngine game;
	while (!game.Tick()) {}
    return 0;
}
