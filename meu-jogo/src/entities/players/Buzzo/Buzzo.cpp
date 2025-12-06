#include "Buzzo.h"
#include <cmath>

Buzzo::Buzzo()
{
    sheetIdle = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/idle.png");
    sheetWalk = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/walking.png");
    sheetRun = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/running.png");
    sheetJump = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/running-jump.png");

    this->sprite = sheetIdle;
    this->w = 64;
    this->h = 64;
    this->maxFrames = 4;
}

void Buzzo::updateDirection()
{
    // Lógica específica de 8 direções do Buzzo
    if (keys[A])
    {
        current_frame_y = 7;
    }
    else if (keys[S])
    {
        if (keys[A])      current_frame_y = 6;
        else if (keys[D]) current_frame_y = 5;
        else              current_frame_y = 4;
    }
    else if (keys[W])
    {
        if (keys[A])      current_frame_y = 3;
        else if (keys[D]) current_frame_y = 2;
        else              current_frame_y = 1;
    }
    else if (keys[D])
    {
        current_frame_y = 0;
    }
}

void Buzzo::updateSpriteSheet() 
{
    if (isJumping)
    {
        sprite = sheetJump;
        maxFrames = 8;
    }
    else if (isMoving)
    {
        if (isRunning)
        {
            sprite = sheetRun;
            maxFrames = 4;
        }
        else
        {
            sprite = sheetWalk;
            maxFrames = 6;
        }
    }
    else
    {
        sprite = sheetIdle;
        maxFrames = 4;
    }
}