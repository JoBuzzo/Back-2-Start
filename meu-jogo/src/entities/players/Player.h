#pragma once
#include "src/core/Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <cmath>

class Player
{
public:
    ALLEGRO_BITMAP *sprite;
    int w, h;
    int current_frame_y;
    float frame;
    int maxFrames;

    int posX, posY;
    bool isMoving;
    bool isRunning;
    bool finished;

    float z;
    float vz;
    bool isJumping;
    float terrainFactor;
    float speed;

    static constexpr float GRAVITY = 0.25f;
    static constexpr float JUMP_FORCE = 5.0f;

    enum KEYS
    {
        W,
        S,
        A,
        D
    };
    bool keys[4] = {false, false, false, false};

    Player() : sprite(nullptr), w(32), h(32), posX(0), posY(0),
               current_frame_y(0), frame(0.f), maxFrames(3),
               isMoving(false), isRunning(false), finished(false),
               z(0.0f), vz(0.0f), isJumping(false),
               terrainFactor(1.0f), speed(2.0f)
    {
    }

    virtual ~Player() { sprite = nullptr; }

    virtual void keyDOWN(int keycode)
    {
        switch (keycode)
        {
        case ALLEGRO_KEY_W:
            keys[W] = true;
            break;
        case ALLEGRO_KEY_S:
            keys[S] = true;
            break;
        case ALLEGRO_KEY_A:
            keys[A] = true;
            break;
        case ALLEGRO_KEY_D:
            keys[D] = true;
            break;
        case ALLEGRO_KEY_LSHIFT:
            isRunning = true;
            break;
        case ALLEGRO_KEY_SPACE:
            jump();
            break;
        }
    }

    virtual void keyUP(int keycode)
    {
        switch (keycode)
        {
        case ALLEGRO_KEY_W:
            keys[W] = false;
            break;
        case ALLEGRO_KEY_S:
            keys[S] = false;
            break;
        case ALLEGRO_KEY_A:
            keys[A] = false;
            break;
        case ALLEGRO_KEY_D:
            keys[D] = false;
            break;
        case ALLEGRO_KEY_LSHIFT:
            isRunning = false;
            break;
        }
    }

    virtual void updateDirection()
    {
        if (keys[S])
            current_frame_y = 0;
        else if (keys[A])
            current_frame_y = 1;
        else if (keys[D])
            current_frame_y = 2;
        else if (keys[W])
            current_frame_y = 3;
    }

    virtual void move()
    {
        if (finished)
            return;
        updateMovingState();

        float currentSpeed = speed * terrainFactor;
        if (isRunning)
            currentSpeed *= 1.5f;
        if (isJumping && isRunning)
            currentSpeed *= 1.2f;

        int dx = 0, dy = 0;
        if (keys[W])
            dy -= (int)currentSpeed;
        if (keys[S])
            dy += (int)currentSpeed;
        if (keys[A])
            dx -= (int)currentSpeed;
        if (keys[D])
            dx += (int)currentSpeed;

        posX += dx;
        posY += dy;

        handleBorderCollision();
        updatePhysics();

        updateDirection();
        updateSpriteSheet();
    }

    void jump()
    {
        if (!isJumping)
        {
            isJumping = true;
            vz = JUMP_FORCE;
            frame = 0;
        }
    }

    void updatePhysics()
    {
        if (isJumping)
        {
            z += vz;
            vz -= GRAVITY;
            if (z <= 0)
            {
                z = 0;
                vz = 0;
                isJumping = false;
            }
        }
    }

    bool handleBorderCollision()
    {
        int hx, hy, hw, hh;
        getHitbox(hx, hy, hw, hh);
        int mapW = WMAP * BLOCKSIZE;
        int mapH = HMAP * BLOCKSIZE;
        bool col = false;

        if (hx < 0)
        {
            posX += (0 - hx);
            col = true;
        }
        else if (hx + hw > mapW)
        {
            posX -= (hx + hw - mapW);
            col = true;
        }

        if (hy < 0)
        {
            posY += (0 - hy);
            col = true;
        }
        else if (hy + hh > mapH)
        {
            posY -= (hy + hh - mapH);
            col = true;
        }
        return col;
    }

    virtual void draw()
    {
        if (finished)
            return;
        if (isMoving || isJumping)
        {
            frame += 0.15f;
            if (frame >= maxFrames)
                frame = 0;
        }
        else
            frame = 0;

        if (sprite)
        {
            if (isJumping)
            {
                float sS = (w / 3) - (z / 4);
                if (sS > 0)
                    al_draw_filled_ellipse(posX + w / 2, posY + h - 5, sS, sS / 2, al_map_rgba(0, 0, 0, 100));
            }
            al_draw_bitmap_region(sprite, w * (int)frame, h * current_frame_y, w, h, posX, posY - (int)z, 0);
        }
    }

    virtual void updateSpriteSheet() {}

    void updateMovingState() { isMoving = (keys[W] || keys[S] || keys[A] || keys[D]); }

virtual void getHitbox(int &x, int &y, int &w, int &h) {
        // --- LÓGICA PADRÃO PARA TODOS OS BICHOS ---
        
        // 1. Define o tamanho da hitbox (Proporcional ao Sprite)
        // Largura: 50% do sprite (evita prender o ombro na parede)
        w = this->w / 2; 
        
        // Altura: 25% do sprite (apenas os pés)
        h = this->h / 4; 

        // 2. Centraliza e Alinha ao Fundo
        // Centraliza no X
        x = posX + (this->w - w) / 2; 
        
        // Empurra para o fundo (Y)
        // (Posição Y do Sprite + Altura Total) - Altura da Hitbox
        y = posY + (this->h - h);     
    }

    virtual void resetPos()
    {
        posX = (WMAP * BLOCKSIZE / 2) - 16;
        posY = HMAP * BLOCKSIZE - 64;
        finished = false;
        z = 0;
        isJumping = false;
    }
    virtual void setPos(int x, int y)
    {
        posX = x;
        posY = y;
    }
    void destroy() { sprite = nullptr; }

    void setNetworkState(float x, float y, int frameY, bool moving, bool finishedState, float netZ, bool netJumping)
    {
        posX = (int)x;
        posY = (int)y;
        current_frame_y = frameY;
        isMoving = moving;
        finished = finishedState;
        z = netZ;
        isJumping = netJumping;
    }
};