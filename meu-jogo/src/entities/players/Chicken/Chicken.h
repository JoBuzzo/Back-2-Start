#pragma once
#include "src/entities/players/Player.h"
#include <allegro5/allegro_image.h>
#include <cstring>

class Chicken : public Player {
public:
    char urlSprite[100] = "assets/sprites/players/chicken.png";

    Chicken();

    void reloadBitmap();
    void keyDOWN(int keycode) override;
    void keyUP(int keycode) override;
    void move() override;
    bool borderCollide();
};
