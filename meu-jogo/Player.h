#pragma once
#include "Config.h"
#include <allegro5/allegro.h>

class Player {
public:
    ALLEGRO_BITMAP* sprite;
    int w, h;
    int posX, posY;
    int current_frame_y;
    float frame;

    enum KEYS { W, S, A, D };
    bool keys[4] = { false, false, false, false };

    Player() : sprite(nullptr), w(32), h(32), posX(0), posY(0), current_frame_y(0), frame(1.f) {}
    virtual ~Player() {
        if (sprite) {
            al_destroy_bitmap(sprite);
            sprite = nullptr;
        }
    }

    virtual void draw() {
        if (keys[W] || keys[S] || keys[A] || keys[D]) {
            frame += 0.1f;
            if (frame > 3) frame -= 3;
        }
        else frame = 1;

        if (sprite)
            al_draw_bitmap_region(sprite, w * (int)frame, current_frame_y, w, h, posX, posY, 0);
    }

    virtual void move() = 0;
    virtual void keyDOWN(int keycode) = 0;
    virtual void keyUP(int keycode) = 0;
    virtual void resetPos() {
        posX = (WMAP * BLOCKSIZE / 2) - 16;
        posY = HMAP * BLOCKSIZE - 64;
	}

    void destroy() {
        if (sprite) {
            al_destroy_bitmap(sprite);
            sprite = nullptr;
        }
    }
};
