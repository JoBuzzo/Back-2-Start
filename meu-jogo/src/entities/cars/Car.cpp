#include "src/entities/cars/Car.h"
#include <cstdio>
#include <vector>
#include "src/managers/resource/ResourceManager.h"

Car::Car()
    : sprite(nullptr), spritePath("test.png"), w(0), h(64), posX(0), posY(0),
    speed(0), movingLeft(false), active(false),
    animated(false), frameCount(1), currentFrame(0.0f)
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
    if (!active || frames.empty()) return;

    if (animated) {
        currentFrame += 0.1f;
        if (currentFrame >= frameCount) currentFrame -= frameCount;
    }

    int index = (int)currentFrame;
    if (index >= frames.size()) index = 0;
    ALLEGRO_BITMAP* currentSprite = frames[index];

    if (movingLeft) {
        al_draw_bitmap(currentSprite, posX, posY, 0);
    }
    else {
        al_draw_bitmap(currentSprite, posX, posY, ALLEGRO_FLIP_HORIZONTAL);
    }
}

void Car::destroy() {

    for (auto f : frames) {
        al_destroy_bitmap(f);
    }
    frames.clear();

    sprite = nullptr;
}

void Car::move() {
    if (!active) return;

    float moveSpeed = std::abs(speed * 2.5);

    if (movingLeft) {
        posX -= moveSpeed;
    }
    else {
        posX += moveSpeed;
    }

    if (movingLeft) {
        if (posX + w < 0) {
            posX = WMAP * BLOCKSIZE;
        }
    }
    else {
        if (posX > WMAP * BLOCKSIZE) {
            posX = -w;
        }
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
        return true;
    }

    return false;
}

bool Car::checkCollision(std::vector<Player*>& players) {
    if (!active) return false;

    for (auto& player : players) {
        if (collide(*player)) {
            return true;
        }
    }
    return false;
}

void Car::reloadBitMap() {
    destroy();

    sprite = ResourceManager::get().getBitmap(spritePath);

    if (!sprite) {
        return;
    }

    int totalWidth = al_get_bitmap_width(sprite);
    int totalHeight = al_get_bitmap_height(sprite);

    if (frameCount < 1) frameCount = 1;

    h = totalHeight;
    w = totalWidth / frameCount;

    for (int i = 0; i < frameCount; i++) {
        ALLEGRO_BITMAP* sub = al_create_sub_bitmap(sprite, i * w, 0, w, h);
        if (sub) {
            frames.push_back(sub);
        }
    }
}
