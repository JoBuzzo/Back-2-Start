#include "Car.h"
#include <cstdio>
#include <vector>

Car::Car()
    : sprite(nullptr), spritePath("test.png"), w(0), h(64), posX(0), posY(0), speed(0), movingLeft(false), active(false)
{
}

void Car::setPosX(int value) {
    posX = BLOCKSIZE * value;
}

void Car::setPosY(int value) {
    posY = BLOCKSIZE * value;
}

void Car::setDirection() {
    posX = BLOCKSIZE * WMAP;
    movingLeft = !movingLeft;
    speed = -speed;
}

void Car::draw() {
    if (!active) return;

    if (movingLeft) {
        al_draw_bitmap(sprite, posX, posY, ALLEGRO_FLIP_HORIZONTAL);
    }
    else {
        al_draw_bitmap(sprite, posX, posY, 0);
    }
}

void Car::destroy() {
    if (sprite) {
        al_destroy_bitmap(sprite);
        sprite = nullptr;
    }
}

void Car::move() {
    if (!active) return;

    posX += speed;

    if (posX > WMAP * BLOCKSIZE) {
        posX = -w;
    }
    else if (posX + w < 0) {
        posX = WMAP * BLOCKSIZE;
    }
    
}

bool Car::collide(Player& player) {
    if (!active) return false;

    int carLeft = posX + 16;
    int carRight = posX + w - 16;
    int carTop = posY + 16;
    int carBottom = posY + h - 16;

    int playerLeft = player.posX;
    int playerRight = player.posX + 32;
    int playerTop = player.posY;
    int playerBottom = player.posY + 32;

    if (carLeft < playerRight && carRight > playerLeft &&
        carTop < playerBottom && carBottom > playerTop)
    {
        player.posX = (WMAP * BLOCKSIZE / 2) - 16;
        player.posY = HMAP * BLOCKSIZE - 64;
        
        return true;
    }

    return false;
}

void Car::collide(std::vector<Player*>& players) {
    if (!active) return;

    bool anyCollide = false;

    for (auto& player : players) {
        if (collide(*player)){
            anyCollide = true;
        }
    }
    if (anyCollide) {
        for (auto& player : players) {
            player->resetPos();
        }
    }
}

void Car::reloadBitMap() {
    sprite = al_load_bitmap(spritePath);
}
