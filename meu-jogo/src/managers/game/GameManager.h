#pragma once
#include <allegro5/allegro_font.h>

#include "src/managers/network/NetworkManager.h"
#include "src/levels/Level.h"
#include "src/entities/players/Player.h"

#include <vector>
#include <string>

// Enums e Constantes
enum GameState { STATE_MENU, STATE_GAME, STATE_PAUSE, STATE_ENDGAME };
const int MSG_LOAD_MAP = 0;
const int MSG_END_GAME = 1;

struct Button {
    int x, y, w, h;
    const char* text;
    bool isOver(float mx, float my);
    void draw(ALLEGRO_FONT* font, bool hover);
};

class GameManager {
private:
    // --- Allegro ---
    ALLEGRO_DISPLAY* display;
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_FONT* font;
    ALLEGRO_FONT* fontSmall;
    ALLEGRO_TRANSFORM trans;

    // --- Estado ---
    bool running;
    bool redraw;
    GameState currentState;
    float gameMouseX, gameMouseY;
    float scale, scaleX, scaleY;

    // --- REDE ---
    NetworkManager net;
    std::string inputIP;

    // --- Transicao ---
    bool isTransitioning;
    float transitionAlpha;
    bool doorClosed;

    // --- Jogo ---
    Level level;
    std::vector<Player*> players;

    // --- UI ---
    Button btnHost, btnJoin, btnExit, btnResume, btnQuit;

    // --- Metodos ---
    bool initAllegro();
    void cleanup();

    void resetPlayersToSpawn();
    void broadcastState();
    void updateGameLogic();    // Logica do Servidor
    void processNetwork();     // Callbacks da Rede
    void handleInput(ALLEGRO_EVENT& ev);
    void draw();

public:
    GameManager();
    ~GameManager();
    void run();
};