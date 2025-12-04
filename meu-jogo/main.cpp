#define _CRT_SECURE_NO_WARNINGS 
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Config.h"
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <enet/enet.h>
#include "NetworkProtocol.h"
#include <string>
#include <algorithm>
#include <string.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// --- INCLUDES DOS OBJETOS ---
#include "Car.h"
#include "Player.h"
#include "BaseMap.h"
#include "Bull.h"
#include "Chicken.h"
#include "Pig.h"
#include "Sheep.h"
#include "Turkey.h"

// --- CONSTANTS ---
const int MSG_LOAD_MAP = 0;
const int MSG_END_GAME = 1;

// Global variable to track server state
std::string serverCurrentLevel = "assets/maps/level1.json";

// --- GAME STATES ---
enum GameState { STATE_MENU, STATE_GAME, STATE_PAUSE, STATE_ENDGAME };

// --- BUTTON STRUCT ---
struct Button {
    int x, y, w, h;
    const char* text;

    bool isOver(float mx, float my) {
        return (mx >= x && mx <= x + w && my >= y && my <= y + h);
    }

    void draw(ALLEGRO_FONT* font, bool hover) {
        al_draw_filled_rectangle(x, y, x + w, y + h, hover ? al_map_rgb(100, 100, 255) : al_map_rgb(50, 50, 50));
        al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255, 255, 255), 2);
        al_draw_text(font, al_map_rgb(255, 255, 255), x + w / 2, y + 10, ALLEGRO_ALIGN_CENTRE, text);
    }
};

// --- GLOBAL NETWORK VARS ---
ENetHost* netHost = nullptr;
ENetPeer* netPeer = nullptr;
bool isServer = false;
int myPlayerId = -1;
int nextPlayerId = 1;

// --- TRANSITION VARS ---
bool isTransitioning = false;
float transitionAlpha = 0.0f;
bool doorClosed = false;

// --- NETWORK HELPER FUNCTIONS ---
bool startHost() {
    isServer = true;
    myPlayerId = 0;
    nextPlayerId = 1;
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    netHost = enet_host_create(&address, 32, 2, 0, 0);
    if (netHost) {
        printf("[NETWORK] Host started on port 1234\n");
        return true;
    }
    return false;
}

bool startClient(std::string ip) {
    isServer = false;
    myPlayerId = -1;
    netHost = enet_host_create(NULL, 1, 2, 0, 0);
    if (!netHost) return false;
    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = 1234;
    netPeer = enet_host_connect(netHost, &address, 2, 0);
    if (netPeer) {
        printf("[NETWORK] Client connecting to %s...\n", ip.c_str());
        return true;
    }
    return false;
}

void disconnectNetwork() {
    if (netPeer) { enet_peer_disconnect_now(netPeer, 0); netPeer = nullptr; }
    if (netHost) { enet_host_destroy(netHost); netHost = nullptr; }
    printf("[NETWORK] Disconnected.\n");
}

void sendInputPacket(int keycode, bool isDown) {
    if (myPlayerId == -1 || !netPeer) return;
    InputPacket pkt;
    pkt.playerId = myPlayerId;
    pkt.keycode = keycode;
    pkt.isDown = isDown;
    ENetPacket* packet = enet_packet_create(&pkt, sizeof(InputPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netPeer, 0, packet);
}

// --- UTILS ---
void resetPlayersToSpawn(BaseMap& map, std::vector<Player*>& pList) {
    for (size_t i = 0; i < pList.size(); i++) {
        pList[i]->finished = false;
        if (i < map.spawnPoints.size()) {
            pList[i]->setPos(map.spawnPoints[i].x, map.spawnPoints[i].y);
        }
        else {
            pList[i]->setPos(100 + (int)i * 32, 100);
        }
    }
}

// --- MAIN FUNCTION ---
int main() {
    // 1. Initialize ENet
    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return 1;
    }
    atexit(enet_deinitialize);

    // 2. Initialize Allegro
    if (!al_init()) return -1;
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();
    al_install_mouse();

    // 3. Display Setup
    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    ALLEGRO_DISPLAY* display = al_create_display(SCREENWIDTH, SCREENHEIGHT);
    al_set_window_title(display, "Back 2 Start");

    ALLEGRO_BITMAP* icon = al_load_bitmap("assets/icon.png");
    if (icon) { al_set_display_icon(display, icon); al_destroy_bitmap(icon); }

    // 4. Scaling Logic
    int monitorW = al_get_display_width(display);
    int monitorH = al_get_display_height(display);
    float sx = monitorW / (float)SCREENWIDTH;
    float sy = monitorH / (float)SCREENHEIGHT;
    float scale = (sx < sy) ? sx : sy;
    float scaleX = (monitorW - (SCREENWIDTH * scale)) / 2;
    float scaleY = (monitorH - (SCREENHEIGHT * scale)) / 2;

    ALLEGRO_TRANSFORM trans;
    al_identity_transform(&trans);
    al_scale_transform(&trans, scale, scale);
    al_translate_transform(&trans, scaleX, scaleY);
    al_use_transform(&trans);

    // 5. Assets
    ALLEGRO_FONT* font = al_load_font("assets/fonts/font.ttf", 25, 0);
    ALLEGRO_FONT* fontSmall = al_load_font("assets/fonts/font.ttf", 18, 0);

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_start_timer(timer);

    // 6. Game Objects
    BaseMap baseMap;
    std::vector<Player*> players;
    GameState currentState = STATE_MENU;
    std::string inputIP = "127.0.0.1";

    // Buttons
    int btnW = 300; int btnH = 50;
    int centerX = SCREENWIDTH / 2 - (btnW / 2);
    Button btnHost = { centerX, 150, btnW, btnH, "CRIAR SERVIDOR" };
    Button btnJoin = { centerX, 350, btnW, btnH, "ENTRAR (CLIENTE)" };
    Button btnExit = { centerX, 450, btnW, btnH, "SAIR DO JOGO" };
    Button btnResume = { centerX, 200, btnW, btnH, "CONTINUAR" };
    Button btnQuit = { centerX, 300, btnW, btnH, "VOLTAR AO MENU" };

    bool redraw = true;
    bool gameRunning = true;
    float gameMouseX = 0, gameMouseY = 0;

    // --- GAME LOOP ---
    while (gameRunning) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;

            // ==========================================
            // LOGIC: TRANSITION (SERVER SIDE)
            // ==========================================
            if (isTransitioning) {
                transitionAlpha += 0.02f;
                if (transitionAlpha >= 1.0f) {
                    transitionAlpha = 1.0f;

                    if (isServer) {
                        if (!baseMap.nextLevelPath.empty()) {
                            // --- CORREÇÃO AQUI ---
                            // 1. Salva o caminho em uma variavel TEMPORARIA antes de carregar
                            std::string levelToLoad = baseMap.nextLevelPath;

                            printf("[SERVER] Loading Next Level: %s\n", levelToLoad.c_str());

                            // 2. Carrega usando a variavel temporaria
                            if (baseMap.loadMapFromJson(levelToLoad)) {

                                // 3. Atualiza a variavel global com o caminho CORRETO (Level 2)
                                // Se usassemos baseMap.nextLevelPath aqui, pegariamos o Level 3!
                                serverCurrentLevel = levelToLoad;

                                // 4. Reset positions
                                resetPlayersToSpawn(baseMap, players);

                                // 5. Tell clients to load map
                                int type = PACKET_CHANGE_LEVEL;
                                int code = MSG_LOAD_MAP;
                                int data[2] = { type, code };
                                ENetPacket* p = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
                                enet_host_broadcast(netHost, 0, p);

                                isTransitioning = false;
                                doorClosed = false;
                                transitionAlpha = 0.0f;
                            }
                            else {
                                printf("[SERVER ERROR] Could not load map file: %s\n", levelToLoad.c_str());
                                isTransitioning = false;
                            }
                        }
                        else {
                            // End Game
                            int type = PACKET_CHANGE_LEVEL;
                            int code = MSG_END_GAME;
                            int data[2] = { type, code };
                            ENetPacket* p = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(netHost, 0, p);
                            enet_host_flush(netHost);

                            currentState = STATE_ENDGAME;
                            isTransitioning = false;
                        }
                    }
                }
            }

            // ==========================================
            // LOGIC: NETWORK HANDLING
            // ==========================================
            if (currentState == STATE_GAME || currentState == STATE_PAUSE || currentState == STATE_ENDGAME) {
                if (netHost) {
                    ENetEvent netEvent;
                    while (enet_host_service(netHost, &netEvent, 0) > 0) {
                        switch (netEvent.type) {
                        case ENET_EVENT_TYPE_CONNECT:
                            if (isServer && nextPlayerId < players.size()) {
                                printf("[SERVER] New Player connected. Sending map: %s\n", serverCurrentLevel.c_str());

                                WelcomePacket wpkt;
                                wpkt.assignedId = nextPlayerId;

                                // Clean memory and copy string
                                memset(wpkt.currentMap, 0, 64);
                                strncpy_s(wpkt.currentMap, sizeof(wpkt.currentMap), serverCurrentLevel.c_str(), 63);

                                ENetPacket* packet = enet_packet_create(&wpkt, sizeof(WelcomePacket), ENET_PACKET_FLAG_RELIABLE);
                                enet_peer_send(netEvent.peer, 0, packet);
                                nextPlayerId++;
                            }
                            break;

                        case ENET_EVENT_TYPE_RECEIVE:
                            if (netEvent.packet->dataLength >= sizeof(int)) {
                                int type = *(int*)netEvent.packet->data;

                                // --- CLIENT LOGIC ---
                                if (!isServer) {
                                    if (type == PACKET_WELCOME) {
                                        WelcomePacket* pkt = (WelcomePacket*)netEvent.packet->data;
                                        myPlayerId = pkt->assignedId;

                                        // *** FIX: HOT JOIN ***
                                        printf("[CLIENT] Joined. Server Map: %s\n", pkt->currentMap);

                                        // Attempt to load the map the server sent
                                        if (baseMap.loadMapFromJson(pkt->currentMap)) {
                                            printf("[CLIENT] Map loaded successfully!\n");
                                            resetPlayersToSpawn(baseMap, players);
                                        }
                                        else {
                                            printf("[CLIENT CRITICAL ERROR] Failed to load map: %s\n", pkt->currentMap);
                                        }
                                    }
                                    else if (type == PACKET_STATE) {
                                        StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                                        if (pkt->id >= 0 && pkt->id < (int)players.size()) {
                                            players[pkt->id]->setNetworkState(pkt->x, pkt->y, pkt->current_frame_y, pkt->isMoving, pkt->isFinished);
                                        }
                                    }
                                    else if (type == PACKET_ENTITY_STATE) {
                                        StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                                        if (pkt->id >= 0 && pkt->id < (int)baseMap.entities.size()) {
                                            Car* car = (Car*)baseMap.entities[pkt->id];
                                            car->posX = (int)pkt->x; car->posY = (int)pkt->y;
                                            car->movingLeft = (pkt->current_frame_y == 1);
                                        }
                                    }
                                    else if (type == PACKET_START_TRANSITION) {
                                        isTransitioning = true;
                                        doorClosed = true;
                                        transitionAlpha = 0.0f;
                                    }
                                    else if (type == PACKET_CHANGE_LEVEL) {
                                        if (netEvent.packet->dataLength >= sizeof(int) * 2) {
                                            int* data = (int*)netEvent.packet->data;
                                            int code = data[1];

                                            if (code == MSG_LOAD_MAP) {
                                                if (!baseMap.nextLevelPath.empty()) {
                                                    printf("[CLIENT] Changing level to: %s\n", baseMap.nextLevelPath.c_str());
                                                    baseMap.loadMapFromJson(baseMap.nextLevelPath);
                                                    resetPlayersToSpawn(baseMap, players);
                                                    isTransitioning = false;
                                                    doorClosed = false;
                                                    transitionAlpha = 0.0f;
                                                }
                                            }
                                            else if (code == MSG_END_GAME) {
                                                currentState = STATE_ENDGAME;
                                            }
                                        }
                                    }
                                }
                                // --- SERVER LOGIC ---
                                else if (isServer && type == PACKET_INPUT) {
                                    InputPacket* pkt = (InputPacket*)netEvent.packet->data;
                                    if (pkt->playerId >= 0 && pkt->playerId < (int)players.size()) {
                                        if (pkt->isDown) players[pkt->playerId]->keyDOWN(pkt->keycode);
                                        else players[pkt->playerId]->keyUP(pkt->keycode);
                                        players[pkt->playerId]->updateMovingState();
                                    }
                                }
                            }
                            enet_packet_destroy(netEvent.packet);
                            break;
                        }
                    }
                }

                // --- GAMEPLAY LOGIC (SERVER ONLY) ---
                if (currentState == STATE_GAME && !isTransitioning) {
                    if (isServer) {
                        int finishedCount = 0;
                        int activePlayers = nextPlayerId;

                        for (int i = 0; i < activePlayers; i++) {
                            players[i]->move();
                            players[i]->updateMovingState();
                            if (!players[i]->finished) {
                                if (baseMap.checkBusCollision(players[i]->posX, players[i]->posY, 32, 32)) players[i]->finished = true;
                            }
                            else {
                                finishedCount++;
                            }
                        }
                        for (auto& e : baseMap.entities) e->move();
                        for (auto& e : baseMap.entities) e->collide(players);

                        if (finishedCount > 0 && finishedCount == activePlayers) {
                            isTransitioning = true;
                            doorClosed = true;
                            transitionAlpha = 0.0f;
                            int type = PACKET_START_TRANSITION;
                            ENetPacket* p = enet_packet_create(&type, sizeof(int), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(netHost, 0, p);
                        }

                        // Broadcast Player States
                        for (int i = 0; i < (int)players.size(); i++) {
                            StatePacket pkt; pkt.type = PACKET_STATE; pkt.id = i;
                            pkt.x = players[i]->posX; pkt.y = players[i]->posY;
                            pkt.current_frame_y = players[i]->current_frame_y; pkt.isMoving = players[i]->isMoving;
                            pkt.isFinished = players[i]->finished;
                            ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
                            enet_host_broadcast(netHost, 0, packet);
                        }
                        // Broadcast Entities (Cars) States
                        for (int i = 0; i < (int)baseMap.entities.size(); i++) {
                            Car* car = (Car*)baseMap.entities[i];
                            StatePacket pkt; pkt.type = PACKET_ENTITY_STATE; pkt.id = i;
                            pkt.x = car->posX; pkt.y = car->posY;
                            pkt.current_frame_y = car->movingLeft ? 1 : 0; pkt.isMoving = true;
                            pkt.isFinished = false;
                            ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
                            enet_host_broadcast(netHost, 0, packet);
                        }
                    }
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            gameRunning = false;
        }
        else if (ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            gameMouseX = (ev.mouse.x - scaleX) / scale;
            gameMouseY = (ev.mouse.y - scaleY) / scale;
        }

        // --- INPUT HANDLING ---
        if (currentState == STATE_MENU) {
            if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (btnHost.isOver(gameMouseX, gameMouseY)) {
                    if (startHost()) {
                        baseMap.loadMapFromJson("assets/maps/level1.json");
                        serverCurrentLevel = "assets/maps/level1.json"; // Reset server map to Level 1

                        players.clear();
                        players.push_back(new Chicken()); players.push_back(new Bull());
                        players.push_back(new Pig()); players.push_back(new Sheep());
                        players.push_back(new Turkey());
                        resetPlayersToSpawn(baseMap, players);
                        currentState = STATE_GAME;
                    }
                }
                else if (btnJoin.isOver(gameMouseX, gameMouseY)) {
                    if (startClient(inputIP)) {
                        // Load Placeholder Map (Level 1) so we don't crash before network data arrives
                        baseMap.loadMapFromJson("assets/maps/level1.json");

                        players.clear();
                        players.push_back(new Chicken()); players.push_back(new Bull());
                        players.push_back(new Pig()); players.push_back(new Sheep());
                        players.push_back(new Turkey());
                        currentState = STATE_GAME;
                    }
                }
                else if (btnExit.isOver(gameMouseX, gameMouseY)) gameRunning = false;
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_CHAR) {
                if (ev.keyboard.unichar >= 32 && ev.keyboard.unichar <= 126 && inputIP.length() < 15)
                    inputIP += (char)ev.keyboard.unichar;
                else if (ev.keyboard.keycode == ALLEGRO_KEY_BACKSPACE && inputIP.length() > 0)
                    inputIP.pop_back();
            }
        }
        else if (currentState == STATE_GAME) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                if (ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) currentState = STATE_PAUSE;
                else {
                    bool isDown = true;
                    if (isServer) {
                        if (myPlayerId >= 0 && myPlayerId < (int)players.size()) {
                            players[myPlayerId]->keyDOWN(ev.keyboard.keycode);
                            players[myPlayerId]->updateMovingState();
                        }
                    }
                    else sendInputPacket(ev.keyboard.keycode, isDown);
                }
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
                bool isDown = false;
                if (isServer) {
                    if (myPlayerId >= 0 && myPlayerId < (int)players.size()) {
                        players[myPlayerId]->keyUP(ev.keyboard.keycode);
                        players[myPlayerId]->updateMovingState();
                    }
                }
                else sendInputPacket(ev.keyboard.keycode, isDown);
            }
        }
        else if (currentState == STATE_PAUSE) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) currentState = STATE_GAME;
            if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
                if (btnResume.isOver(gameMouseX, gameMouseY)) currentState = STATE_GAME;
                else if (btnQuit.isOver(gameMouseX, gameMouseY)) {
                    disconnectNetwork();
                    for (auto p : players) delete p; players.clear();
                    currentState = STATE_MENU;
                }
            }
        }
        else if (currentState == STATE_ENDGAME) {
            if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                disconnectNetwork();
                for (auto p : players) delete p; players.clear();
                currentState = STATE_MENU;
            }
        }

        // --- DRAWING ---
        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(0, 0, 0));

            if (currentState == STATE_MENU) {
                al_draw_text(font, al_map_rgb(50, 50, 50), SCREENWIDTH / 2 + 3, 53, ALLEGRO_ALIGN_CENTRE, "BACK 2 START");
                al_draw_text(font, al_map_rgb(255, 255, 0), SCREENWIDTH / 2, 50, ALLEGRO_ALIGN_CENTRE, "BACK 2 START");

                btnHost.draw(font, btnHost.isOver(gameMouseX, gameMouseY));
                btnJoin.draw(font, btnJoin.isOver(gameMouseX, gameMouseY));
                btnExit.draw(font, btnExit.isOver(gameMouseX, gameMouseY));

                al_draw_text(font, al_map_rgb(200, 200, 200), SCREENWIDTH / 2, 240, ALLEGRO_ALIGN_CENTRE, "Digite o IP do Host:");
                al_draw_rectangle(SCREENWIDTH / 2 - 150, 270, SCREENWIDTH / 2 + 150, 310, al_map_rgb(255, 255, 255), 2);
                al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, 275, ALLEGRO_ALIGN_CENTRE, inputIP.c_str());
            }
            else if (currentState == STATE_GAME || currentState == STATE_PAUSE) {
                baseMap.drawMap();
                baseMap.drawBus(doorClosed);
                for (auto& e : baseMap.entities) e->draw();
                for (auto& p : players) p->draw();

                if (fontSmall) al_draw_text(fontSmall, al_map_rgb(255, 255, 255), SCREENWIDTH - 10, 10, ALLEGRO_ALIGN_RIGHT, baseMap.title.c_str());

                if (myPlayerId == -1) al_draw_text(font, al_map_rgb(255, 0, 0), 10, 10, 0, "CONECTANDO...");
                else al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 10, 0, "Player ID: %d", myPlayerId);

                if (transitionAlpha > 0.0f) al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba_f(0, 0, 0, transitionAlpha));

                if (currentState == STATE_PAUSE) {
                    al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba(0, 0, 0, 150));
                    al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, 100, ALLEGRO_ALIGN_CENTRE, "PAUSADO");
                    btnResume.draw(font, btnResume.isOver(gameMouseX, gameMouseY));
                    btnQuit.draw(font, btnQuit.isOver(gameMouseX, gameMouseY));
                }
            }
            else if (currentState == STATE_ENDGAME) {
                al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, SCREENHEIGHT / 2 - 20, ALLEGRO_ALIGN_CENTRE, "PARABENS!");
                al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2 + 20, ALLEGRO_ALIGN_CENTRE, "VOCES COMPLETARAM O JOGO.");
                al_draw_text(font, al_map_rgb(100, 100, 100), SCREENWIDTH / 2, SCREENHEIGHT - 50, ALLEGRO_ALIGN_CENTRE, "Pressione ESC para voltar");
            }
            al_flip_display();
        }
    }

    // --- CLEANUP ---
    disconnectNetwork();
    if (netHost) enet_host_destroy(netHost);
    for (int i = 0; i < (int)baseMap.tileNames.size(); i++) if (baseMap.tiles[i]) al_destroy_bitmap(baseMap.tiles[i]);
    for (auto& e : baseMap.entities) { e->destroy(); delete e; }
    for (auto& p : players) { p->destroy(); delete p; }
    if (fontSmall) al_destroy_font(fontSmall);
    al_destroy_display(display);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}