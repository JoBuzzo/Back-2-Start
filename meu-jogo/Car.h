#pragma once
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <string>
#include <vector>
#include "Player.h"
#include "Config.h"

class Car {
public:
    ALLEGRO_BITMAP* sprite;
    std::vector<ALLEGRO_BITMAP*> frames;

    std::string spritePath;

    int w, h;
    int posX, posY;
    float speed;
    bool movingLeft;
    bool active;

    bool animated;
    int frameCount;
    float currentFrame; 

    Car();
    virtual ~Car() = default;

    void setPosX(int value);
    void setPosY(int value);
    void setDirection();

    virtual void draw();
    virtual void destroy();
    virtual void move();

    virtual bool collide(Player& player);
    virtual void collide(std::vector<Player*>& players);

    virtual void reloadBitMap();
};