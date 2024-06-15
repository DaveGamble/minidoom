#include <iostream>

#include "DoomEngine.h"

int main(int argc, char* argv[])
{
    DoomEngine game;
    game.Init();

    while (!game.IsOver())
    {
        game.ProcessInput();
        game.Update();
        game.Render();
        game.Delay();
    }

    return 0;
}
