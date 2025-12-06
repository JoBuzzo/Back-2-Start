#include "src/entities/players/Turkey/Turkey.h"
#include "src/managers/resource/ResourceManager.h"

Turkey::Turkey() {
    sprite = ResourceManager::get().getBitmap("assets/sprites/players/turkey.png");
	maxFrames = 6;
}

void Turkey::updateDirection() {
    if (keys[S])      current_frame_y = 0;
    else if (keys[A]) current_frame_y = 2;
    else if (keys[D]) current_frame_y = 3;
    else if (keys[W]) current_frame_y = 1;
}