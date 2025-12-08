#include "src/entities/Entity.h"
#include "src/managers/resource/ResourceManager.h"
#include "src/entities/players/Player.h"
#include <cmath>

Entity::Entity()
    : sprite(nullptr), w(0), h(0), posX(0), posY(0),
      speed(0), movingLeft(false), active(false),
      animated(false), frameCount(1), currentFrame(0.0f),
      hitboxOffsetLeft(0),hitboxOffsetRight(), hitboxOffsetTop(0), hitboxOffsetBottom(0)
{
}

Entity::~Entity() {
    destroy();
}

void Entity::setPosX(int value) { posX = (float)(BLOCKSIZE * value); }
void Entity::setPosY(int value) { posY = (float)(BLOCKSIZE * value); }

void Entity::setDirection() {
    posX = (float)(BLOCKSIZE * WMAP);
    movingLeft = !movingLeft;
    speed = -speed;
}

void Entity::load() {
    destroy();
    sprite = ResourceManager::get().getBitmap(spritePath);

    if (!sprite) return;

    int totalWidth = al_get_bitmap_width(sprite);
    h = al_get_bitmap_height(sprite);
    
    if (frameCount < 1) frameCount = 1;
    w = totalWidth / frameCount;

    for (int i = 0; i < frameCount; i++) {
        ALLEGRO_BITMAP* sub = al_create_sub_bitmap(sprite, i * w, 0, w, h);
        if (sub) frames.push_back(sub);
    }
}

void Entity::draw() {
    if (!active || frames.empty()) return;

    if (animated) {
        currentFrame += 0.1f;
        if (currentFrame >= frameCount) currentFrame -= frameCount;
    }
    int index = (int)currentFrame;
    if (index >= frames.size()) index = 0;

    int flags = movingLeft ? 0 : ALLEGRO_FLIP_HORIZONTAL;
    al_draw_bitmap(frames[index], posX, posY, flags);

    if (debugMode) drawHitbox(); 
}

void Entity::update() {
    if (!active) return;

    float moveSpeed = std::abs(speed * 2.5f);
    
    if (movingLeft) posX -= moveSpeed;
    else posX += moveSpeed;

    if (movingLeft) {
        if (posX + w < 0) posX = (float)(WMAP * BLOCKSIZE);
    }
    else {
        if (posX > WMAP * BLOCKSIZE) posX = (float)(-w);
    }
}

bool Entity::collide(Player& player) {
    if (!active) return false;
    
    int realOffsetLeft = movingLeft ? hitboxOffsetRight : hitboxOffsetLeft;
    int realOffsetRight = movingLeft ? hitboxOffsetLeft : hitboxOffsetRight;

    // Cálculos
    int entityLeft   = (int)posX + realOffsetLeft;
    int entityRight  = (int)posX + w - realOffsetRight;
    int entityTop    = (int)posY + hitboxOffsetTop;
    int entityBottom = (int)posY + h - hitboxOffsetBottom;

    int pX, pY, pW, pH;
    player.getHitbox(pX, pY, pW, pH);

    // Verifica intersecção AABB padrão
    if (entityLeft < pX + pW && entityRight > pX &&
        entityTop < pY + pH && entityBottom > pY)
    {        
        return true;
    }
    return false;
}

bool Entity::checkCollision(std::vector<Player*>& players) {
    if (!active) return false;
    for (auto& player : players) {
        if (collide(*player)) return true;
    }
    return false;
}

void Entity::drawHitbox() {
    if (!active) return;

    int realOffsetLeft = movingLeft ? hitboxOffsetRight : hitboxOffsetLeft;
    int realOffsetRight = movingLeft ? hitboxOffsetLeft : hitboxOffsetRight;

    float x1 = posX + realOffsetLeft;
    float y1 = posY + hitboxOffsetTop;
    float x2 = posX + w - realOffsetRight;
    float y2 = posY + h - hitboxOffsetBottom;

    al_draw_rectangle(x1, y1, x2, y2, al_map_rgb(255, 0, 0), 1);
}

void Entity::destroy() {
    for (auto f : frames) al_destroy_bitmap(f);
    frames.clear();
    sprite = nullptr;
}