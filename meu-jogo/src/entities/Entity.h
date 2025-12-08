#pragma once
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <string>
#include <vector>
#include "src/core/Config.h"


class Player; 

class Entity {
public:
    bool debugMode =  false;

    ALLEGRO_BITMAP* sprite;
    std::vector<ALLEGRO_BITMAP*> frames;
    std::string spritePath;

    int w, h;
    float posX, posY;
    float speed;
    bool movingLeft;
    bool active;
    
    bool animated;
    int frameCount;
    float currentFrame;


    int hitboxOffsetLeft;
    int hitboxOffsetRight;
    int hitboxOffsetTop;
    int hitboxOffsetBottom;

    Entity();
    virtual ~Entity();      

    void setPosX(int value);
    void setPosY(int value);
    void setDirection();
    void setActive(bool value) { active = value; }

    virtual void load();
    virtual void update();
    virtual void draw();
    virtual void destroy();

    virtual bool collide(Player& player);
    virtual bool checkCollision(std::vector<Player*>& players);
    
    void drawHitbox();
};