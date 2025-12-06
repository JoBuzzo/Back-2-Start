#include "src/entities/players/Bull/Bull.h"
#include "src/managers/resource/ResourceManager.h"

Bull::Bull()
{
    w = 64;
    h = 64;
    sprite = ResourceManager::get().getBitmap("assets/sprites/players/bull.png");
    maxFrames = 6;
}

void Bull::updateDirection()
{

    if (keys[S])
        current_frame_y = 0;
    else if (keys[A])
        current_frame_y = 2;
    else if (keys[D])
        current_frame_y = 3;
    else if (keys[W])
        current_frame_y = 1;
}
