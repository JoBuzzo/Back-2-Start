#include "src/entities/players/Buzzo/Buzzo.h"
#include <allegro5/allegro_primitives.h>
#include <cmath>

Buzzo::Buzzo() {
    sheetIdle = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/idle.png");
    sheetWalk = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/walking.png");
    sheetRun  = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/running.png");
    sheetJump = ResourceManager::get().getBitmap("assets/sprites/players/buzzo/running-jump.png");

    this->sprite = sheetIdle;
    this->w = 64;
    this->h = 64;
    
    this->posX = 100;
    this->posY = 100;
    
    this->speed = 2.0f;
    this->maxFrames = 4;
    this->isRunning = false;

    this->z = 0.0f;
    this->vz = 0.0f;
    this->isJumping = false;
}

Buzzo::~Buzzo() {

}

void Buzzo::keyDOWN(int keycode) {
    switch (keycode) {
        case ALLEGRO_KEY_W: keys[W] = true; break;
        case ALLEGRO_KEY_S: keys[S] = true; break;
        case ALLEGRO_KEY_A: keys[A] = true; break;
        case ALLEGRO_KEY_D: keys[D] = true; break;
        case ALLEGRO_KEY_LSHIFT: isRunning = true; break;
        case ALLEGRO_KEY_SPACE: 
            if (!isJumping) {
                isJumping = true;
                vz = JUMP_FORCE;
                frame = 0;
            }
            break;
    }
}

void Buzzo::keyUP(int keycode) {
    switch (keycode) {
        case ALLEGRO_KEY_W: keys[W] = false; break;
        case ALLEGRO_KEY_S: keys[S] = false; break;
        case ALLEGRO_KEY_A: keys[A] = false; break;
        case ALLEGRO_KEY_D: keys[D] = false; break;
        case ALLEGRO_KEY_LSHIFT: isRunning = false; break;
    }
}

void Buzzo::updateDirection() {    
    if (keys[A]) {
        current_frame_y = 7;
    } else
    if (keys[S]) {
        if (keys[A])      current_frame_y = 6;
        else if (keys[D]) current_frame_y = 5;
        else              current_frame_y = 4;
    }
    else if (keys[W]) {
        if (keys[A])      current_frame_y = 3;
        else if (keys[D]) current_frame_y = 2;
        else              current_frame_y = 1;
    }
    else if (keys[D]) {
        current_frame_y = 0;
    }
}

void Buzzo::move() {
    if (finished) return;

    updateMovingState();

    float moveSpeed = speed;
    if (isRunning) {
        moveSpeed = isJumping ? speed * 2.5f : speed * 1.5f;
    }
    
    float dx = 0, dy = 0;
    if (keys[W]) dy -= moveSpeed;
    if (keys[S]) dy += moveSpeed;
    if (keys[A]) dx -= moveSpeed;
    if (keys[D]) dx += moveSpeed;

    posX += (int)dx;
    posY += (int)dy;

    if (isJumping) {
        z += vz; 
        vz -= GRAVITY;
        
        if (z <= 0) {
            z = 0;
            vz = 0;
            isJumping = false;
        }
    }

    updateDirection(); 

    if (isJumping) {
        sprite = sheetJump;
        maxFrames = 8;
    }
    else if (isMoving) {
        if (isRunning) {
            sprite = sheetRun;
            maxFrames = 4;
        } else {
            sprite = sheetWalk;
            maxFrames = 6;
        }
    } else {
        sprite = sheetIdle;
        maxFrames = 4;
    }
}

void Buzzo::draw() {
    if (finished) return;

    if (isMoving || isJumping || sprite == sheetIdle) { 
        frame += 0.15f;
        if (frame >= maxFrames) {
            frame = 0; 
        }
    } else {
        frame = 0;
    }

    if (sprite) {
        if (isJumping) {
            float shadowSize = (w / 3) - (z / 4); 
            if (shadowSize > 0)
                al_draw_filled_ellipse(posX + w/2, posY + h - 5, shadowSize, shadowSize/2, al_map_rgba(0,0,0,100));
        }

        al_draw_bitmap_region(sprite, 
            w * (int)frame,
            h * current_frame_y,
            w, h,
            posX, posY - (int)z, 0);
    }

}

void Buzzo::getHitbox(int& x, int& y, int& w, int& h) {

    x = posX + 16; 
    y = posY + 32;
    w = 32;
    h = 32;
}