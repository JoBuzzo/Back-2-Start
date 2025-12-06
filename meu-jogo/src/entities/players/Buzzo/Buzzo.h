#pragma once
#include "src/entities/players/Player.h"
#include "src/managers/resource/ResourceManager.h"

class Buzzo : public Player
{
private:
    ALLEGRO_BITMAP *sheetIdle;
    ALLEGRO_BITMAP *sheetWalk;
    ALLEGRO_BITMAP *sheetRun;
    ALLEGRO_BITMAP *sheetJump;

public:
    Buzzo();
    
    void updateDirection() override;

    void updateSpriteSheet() override;
};