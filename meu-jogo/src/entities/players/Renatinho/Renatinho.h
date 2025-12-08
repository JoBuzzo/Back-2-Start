#pragma once
#include "src/entities/players/Player.h"
#include "src/managers/resource/ResourceManager.h"

class Renatinho : public Player
{
private:
    ALLEGRO_BITMAP *sheetIdle;
    ALLEGRO_BITMAP *sheetWalk;
    ALLEGRO_BITMAP *sheetRun;
    ALLEGRO_BITMAP *sheetJump;

public:
    Renatinho();
    
    void updateDirection() override;

    void updateSpriteSheet() override;
};