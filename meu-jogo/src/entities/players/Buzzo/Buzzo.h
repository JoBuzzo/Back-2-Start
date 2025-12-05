#pragma once
#include "src/entities/players/Player.h"
#include "src/managers/resource/ResourceManager.h"

class Buzzo : public Player {
private:
    ALLEGRO_BITMAP* sheetIdle;
    ALLEGRO_BITMAP* sheetWalk;
    ALLEGRO_BITMAP* sheetRun;
    ALLEGRO_BITMAP* sheetJump;

    int maxFrames;
    float speed;
    bool isRunning;
    float z;
    float vz;
    bool isJumping;

    void updateDirection();

public:
    Buzzo();
    virtual ~Buzzo();

    void move() override;
    void draw() override; 
    void keyDOWN(int keycode) override;
    void keyUP(int keycode) override;
    void getHitbox(int& x, int& y, int& w, int& h) override;
};