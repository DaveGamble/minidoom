This is a repo for a fork of DIYDoom [https://github.com/amroibrahim/DIYDoom/tree/master].

It's in a very rough state, and includes SDL2 directly. It may make your eyes bleed. Tidying is for later.

TODO:
Graphics:
- Render sprites
- Better lighting effects (more like the original)
Optimising:
- Optimise floor/ceiling drawing
- Memory usage
- What can I be precomputing?
Gamey stuff:
- View bob?
- SFX / Music
- A nice interface for selecting weapons and drawing firing animations?
- Be really cool if we could do, like, doors and floor movements and stuff?


Build is for Xcode on Apple Silicon macs (but if you replaced the precompiled SDL binary you could do whatever)

Requires a WAD file next to the binary or crashy times. See DIYDoom.cpp

