#pragma once
#include <allegro5/color.h>

enum GameState {
    STATE_MENU,
    STATE_GAME,
    STATE_PAUSE
};

struct Button {
    int x, y, w, h;
    const char* text;
    ALLEGRO_COLOR color;

    bool isHover(int mouseX, int mouseY) {
        return (mouseX >= x && mouseX <= x + w && mouseY >= y && mouseY <= y + h);
    }
};