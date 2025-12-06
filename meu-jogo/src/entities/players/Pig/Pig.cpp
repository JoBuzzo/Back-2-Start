#include "src/entities/players/Pig/Pig.h"
#include "src/managers/resource/ResourceManager.h"

Pig::Pig() {
    sprite = ResourceManager::get().getBitmap("assets/sprites/players/pig.png");
	maxFrames = 6;
}

void Pig::updateDirection() {
    
    if (keys[S])      current_frame_y = 0;
    else if (keys[A]) current_frame_y = 2;
    else if (keys[D]) current_frame_y = 3;
    else if (keys[W]) current_frame_y = 1;
}
