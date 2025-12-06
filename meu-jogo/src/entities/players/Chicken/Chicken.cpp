#include "Chicken.h"
#include "src/managers/resource/ResourceManager.h"

Chicken::Chicken() {
    sprite = ResourceManager::get().getBitmap("assets/sprites/players/chicken.png");
    maxFrames = 3;
}

void Chicken::updateDirection() {
    
    if (keys[S])      current_frame_y = 2;
    else if (keys[A]) current_frame_y = 3;
    else if (keys[D]) current_frame_y = 1;
    else if (keys[W]) current_frame_y = 0;
}