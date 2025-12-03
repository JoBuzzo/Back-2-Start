#pragma once
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include "Player.h"
#include "Config.h"

class Car {
public:
    ALLEGRO_BITMAP* sprite;
    const char* spritePath;

    int w, h;
    int posX, posY;
    float speed;
    bool movingLeft;
    bool active;

    Car();
    virtual ~Car() = default;

    void setPosX(int value);
    void setPosY(int value);
    void setDirection();

    virtual void draw();
    virtual void destroy();
    virtual void move();
    virtual void collide(Player& player);
    virtual void reloadBitMap();
};
