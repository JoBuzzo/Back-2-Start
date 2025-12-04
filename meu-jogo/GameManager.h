#pragma once
#include "Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <enet/enet.h>
#include "NetworkProtocol.h"
#include "BaseMap.h"
#include "Player.h"
#include <vector>
#include <string>

// Enums e Structs auxiliares
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

    float scale;
    float scaleX;
    float scaleY;

    // --- Allegro ---
    ALLEGRO_DISPLAY* display;
    ALLEGRO_EVENT_QUEUE* queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_FONT* font;
    ALLEGRO_FONT* fontSmall;
    ALLEGRO_TRANSFORM trans;

    // --- Estado do Jogo ---
    bool running;
    bool redraw;
    GameState currentState;
    float gameMouseX, gameMouseY;

    // --- Rede ---
    ENetHost* netHost;
    ENetPeer* netPeer;
    bool isServer;
    int myPlayerId;
    int nextPlayerId;
    std::string inputIP;

    // --- Transição ---
    bool isTransitioning;
    float transitionAlpha;
    bool doorClosed;

    // --- Objetos do Mundo ---
    BaseMap baseMap;
    std::vector<Player*> players;

    // --- UI ---
    Button btnHost, btnJoin, btnExit, btnResume, btnQuit;

    // --- Métodos Internos ---
    bool initAllegro();
    void initNetwork(); // Inicializa lib ENet
    void cleanup();

    // Helpers de Rede
    bool startHost();
    bool startClient(std::string ip);
    void disconnectNetwork();
    void sendInputPacket(int keycode, bool isDown);
    void processNetworkEvents();
    void broadcastState();

    // Lógica de Jogo
    void resetPlayersToSpawn();
    void updateGameLogic();
    void handleInput(ALLEGRO_EVENT& ev);
    void draw();

public:
    GameManager();
    ~GameManager();
    void run();
};