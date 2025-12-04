#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "GameManager.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <cstdio>

// Include dos bichos específicos
#include "Bull.h"
#include "Chicken.h"
#include "Pig.h"
#include "Sheep.h"
#include "Turkey.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// --- Implementação do Button ---
bool Button::isOver(float mx, float my) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void Button::draw(ALLEGRO_FONT* font, bool hover) {
    al_draw_filled_rectangle(x, y, x + w, y + h, hover ? al_map_rgb(100, 100, 255) : al_map_rgb(50, 50, 50));
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255, 255, 255), 2);
    al_draw_text(font, al_map_rgb(255, 255, 255), x + w / 2, y + 10, ALLEGRO_ALIGN_CENTRE, text);
}

// --- Construtor ---
GameManager::GameManager() {
    display = nullptr; queue = nullptr; timer = nullptr; font = nullptr; fontSmall = nullptr;
    netHost = nullptr; netPeer = nullptr;

    scale = 1.0f;
    scaleX = 0.0f;
    scaleY = 0.0f;

    running = true; redraw = true;
    currentState = STATE_MENU;

    isServer = false; myPlayerId = -1; nextPlayerId = 1;
    inputIP = "127.0.0.1";

    isTransitioning = false; transitionAlpha = 0.0f; doorClosed = false;
    gameMouseX = 0; gameMouseY = 0;

    // Configura Botões
    int btnW = 300; int btnH = 50;
    int centerX = SCREENWIDTH / 2 - (btnW / 2);
    btnHost = { centerX, 150, btnW, btnH, "CRIAR SERVIDOR" };
    btnJoin = { centerX, 350, btnW, btnH, "ENTRAR (CLIENTE)" };
    btnExit = { centerX, 450, btnW, btnH, "SAIR DO JOGO" };
    btnResume = { centerX, 200, btnW, btnH, "CONTINUAR" };
    btnQuit = { centerX, 300, btnW, btnH, "VOLTAR AO MENU" };
}

GameManager::~GameManager() {
    cleanup();
}

// --- Inicialização ---
bool GameManager::initAllegro() {
    if (!al_init()) return false;
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();
    al_install_mouse();

    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    display = al_create_display(SCREENWIDTH, SCREENHEIGHT);
    al_set_window_title(display, "Back 2 Start");

    ALLEGRO_BITMAP* icon = al_load_bitmap("assets/icon.png");
    if (icon) { al_set_display_icon(display, icon); al_destroy_bitmap(icon); }

    // Zoom
    int monitorW = al_get_display_width(display);
    int monitorH = al_get_display_height(display);
    float sx = monitorW / (float)SCREENWIDTH;
    float sy = monitorH / (float)SCREENHEIGHT;
	scale = sx < sy ? sx : sy;
    scaleX = (monitorW - (SCREENWIDTH * scale)) / 2;
    scaleY = (monitorH - (SCREENHEIGHT * scale)) / 2;

    al_identity_transform(&trans);
    al_scale_transform(&trans, scale, scale);
    al_translate_transform(&trans, scaleX, scaleY);
    al_use_transform(&trans);


    font = al_load_font("assets/fonts/font.ttf", 25, 0);
    fontSmall = al_load_font("assets/fonts/font.ttf", 18, 0);

    timer = al_create_timer(1.0 / 60.0);
    queue = al_create_event_queue();

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_start_timer(timer);

    return true;
}

void GameManager::initNetwork() {
    if (enet_initialize() != 0) {
        printf("Erro fatal ao iniciar ENet.\n");
        exit(1);
    }
    atexit(enet_deinitialize);
}

void GameManager::run() {
    initNetwork();
    if (!initAllegro()) return;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;

            if (isTransitioning) {
                transitionAlpha += 0.02f;
                if (transitionAlpha >= 1.0f) {
                    transitionAlpha = 1.0f;
                    if (isServer) {
                        if (!baseMap.nextLevelPath.empty()) {
                            baseMap.loadMapFromJson(baseMap.nextLevelPath);
                            resetPlayersToSpawn();

                            int type = PACKET_CHANGE_LEVEL;
                            int code = MSG_LOAD_MAP;
                            int data[2] = { type, code };
                            ENetPacket* p = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(netHost, 0, p);

                            isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                        }
                        else {
                            int type = PACKET_CHANGE_LEVEL;
                            int code = MSG_END_GAME;
                            int data[2] = { type, code };
                            ENetPacket* p = enet_packet_create(data, sizeof(data), ENET_PACKET_FLAG_RELIABLE);
                            enet_host_broadcast(netHost, 0, p);
                            enet_host_flush(netHost);
                            currentState = STATE_ENDGAME; isTransitioning = false;
                        }
                    }
                }
            }

            if (currentState == STATE_GAME || currentState == STATE_PAUSE || currentState == STATE_ENDGAME) {
                if (netHost) processNetworkEvents();

                if (currentState == STATE_GAME && !isTransitioning) {
                    updateGameLogic();
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            running = false;
        }
        else {
            handleInput(ev);
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false;
            draw();
            al_flip_display();
        }
    }
}

// --- Rede ---
bool GameManager::startHost() {
    isServer = true;
    myPlayerId = 0;
    nextPlayerId = 1;
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = 1234;
    netHost = enet_host_create(&address, 32, 2, 0, 0);
    return (netHost != NULL);
}

bool GameManager::startClient(std::string ip) {
    isServer = false;
    myPlayerId = -1;
    netHost = enet_host_create(NULL, 1, 2, 0, 0);
    if (!netHost) return false;
    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = 1234;
    netPeer = enet_host_connect(netHost, &address, 2, 0);
    return (netPeer != NULL);
}

void GameManager::disconnectNetwork() {
    if (netPeer) { enet_peer_disconnect_now(netPeer, 0); netPeer = nullptr; }
    if (netHost) { enet_host_destroy(netHost); netHost = nullptr; }
}

void GameManager::sendInputPacket(int keycode, bool isDown) {
    if (myPlayerId == -1 || !netPeer) return;
    InputPacket pkt;
    pkt.playerId = myPlayerId;
    pkt.keycode = keycode;
    pkt.isDown = isDown;
    ENetPacket* packet = enet_packet_create(&pkt, sizeof(InputPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netPeer, 0, packet);
}

void GameManager::processNetworkEvents() {
    ENetEvent netEvent;
    while (enet_host_service(netHost, &netEvent, 0) > 0) {
        switch (netEvent.type) {
        case ENET_EVENT_TYPE_CONNECT:
            if (isServer && nextPlayerId < players.size()) {
                WelcomePacket wpkt;
                wpkt.assignedId = nextPlayerId;
                std::string currentMap = baseMap.path.empty() ? "assets/maps/level1.json" : baseMap.path;
                strncpy_s(wpkt.currentLevelPath, sizeof(wpkt.currentLevelPath), currentMap.c_str(), 127);

                ENetPacket* packet = enet_packet_create(&wpkt, sizeof(WelcomePacket), ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(netEvent.peer, 0, packet);
                nextPlayerId++;
            }
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            if (netEvent.packet->dataLength >= sizeof(int)) {
                int type = *(int*)netEvent.packet->data;
                if (!isServer) { // Cliente
                    if (type == PACKET_WELCOME) {
                        WelcomePacket* pkt = (WelcomePacket*)netEvent.packet->data;
                        myPlayerId = pkt->assignedId;
                        printf("[CLIENT] Loading Map: %s\n", pkt->currentLevelPath);
                        baseMap.loadMapFromJson(pkt->currentLevelPath);
                    }
                    else if (type == PACKET_STATE) {
                        StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                        if (pkt->id >= 0 && pkt->id < players.size())
                            players[pkt->id]->setNetworkState(pkt->x, pkt->y, pkt->current_frame_y, pkt->isMoving, pkt->isFinished);
                    }
                    else if (type == PACKET_ENTITY_STATE) {
                        StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                        if (pkt->id >= 0 && pkt->id < baseMap.entities.size()) {
                            Car* car = (Car*)baseMap.entities[pkt->id];
                            car->posX = (int)pkt->x; car->posY = (int)pkt->y;
                            car->movingLeft = (pkt->current_frame_y == 1);
                        }
                    }
                    else if (type == PACKET_START_TRANSITION) {
                        isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
                    }
                    else if (type == PACKET_CHANGE_LEVEL && netEvent.packet->dataLength >= sizeof(int) * 2) {
                        int* data = (int*)netEvent.packet->data;
                        int code = data[1];
                        if (code == MSG_LOAD_MAP) {
                            if (!baseMap.nextLevelPath.empty()) {
                                baseMap.loadMapFromJson(baseMap.nextLevelPath);
                                isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                            }
                        }
                        else if (code == MSG_END_GAME) {
                            currentState = STATE_ENDGAME;
                            isTransitioning = false;
                            transitionAlpha = 0.0f;
                        }
                    }
                }
                else if (isServer && type == PACKET_INPUT) { // Servidor
                    InputPacket* pkt = (InputPacket*)netEvent.packet->data;
                    if (pkt->playerId >= 0 && pkt->playerId < players.size()) {
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

// --- Logica do Jogo ---
void GameManager::updateGameLogic() {
    if (!isServer) return;

    int finishedCount = 0;
    int activePlayers = nextPlayerId;

    for (int i = 0; i < activePlayers; i++) {
        players[i]->move();
        players[i]->updateMovingState();
        if (!players[i]->finished) {
            if (baseMap.checkBusCollision(players[i]->posX, players[i]->posY, 32, 32)) players[i]->finished = true;
        }
        else finishedCount++;
    }

    for (auto& e : baseMap.entities) e->move();
    for (auto& e : baseMap.entities) e->collide(players);

    if (finishedCount > 0 && finishedCount == activePlayers) {
        isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
        int type = PACKET_START_TRANSITION;
        ENetPacket* p = enet_packet_create(&type, sizeof(int), ENET_PACKET_FLAG_RELIABLE);
        enet_host_broadcast(netHost, 0, p);
    }

    broadcastState();
}

void GameManager::broadcastState() {
    for (int i = 0; i < players.size(); i++) {
        StatePacket pkt; pkt.type = PACKET_STATE; pkt.id = i;
        pkt.x = players[i]->posX; pkt.y = players[i]->posY;
        pkt.current_frame_y = players[i]->current_frame_y; pkt.isMoving = players[i]->isMoving;
        pkt.isFinished = players[i]->finished;
        ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_host_broadcast(netHost, 0, packet);
    }
    for (int i = 0; i < baseMap.entities.size(); i++) {
        Car* car = (Car*)baseMap.entities[i];
        StatePacket pkt; pkt.type = PACKET_ENTITY_STATE; pkt.id = i;
        pkt.x = car->posX; pkt.y = car->posY;
        pkt.current_frame_y = car->movingLeft ? 1 : 0; pkt.isMoving = true;
        pkt.isFinished = false;
        ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
        enet_host_broadcast(netHost, 0, packet);
    }
}

void GameManager::handleInput(ALLEGRO_EVENT& ev) {
    if (ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
        gameMouseX = (ev.mouse.x - scaleX) / scale;
        gameMouseY = (ev.mouse.y - scaleY) / scale;
    }

    if (currentState == STATE_MENU) {
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (btnHost.isOver(gameMouseX, gameMouseY)) {
                if (startHost()) {
                    baseMap.loadMapFromJson("assets/maps/level1.json");
                    players.clear();
                    players.push_back(new Chicken()); players.push_back(new Bull());
                    players.push_back(new Pig()); players.push_back(new Sheep()); players.push_back(new Turkey());
                    resetPlayersToSpawn();
                    currentState = STATE_GAME;
                }
            }
            else if (btnJoin.isOver(gameMouseX, gameMouseY)) {
                if (startClient(inputIP)) {
                    // Espera WelcomePacket
                    players.clear();
                    players.push_back(new Chicken()); players.push_back(new Bull());
                    players.push_back(new Pig()); players.push_back(new Sheep()); players.push_back(new Turkey());
                    currentState = STATE_GAME;
                }
            }
            else if (btnExit.isOver(gameMouseX, gameMouseY)) running = false;
        }
        // Key Char logic (inputIP)...
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
                if (isServer && myPlayerId >= 0) {
                    players[myPlayerId]->keyDOWN(ev.keyboard.keycode);
                    players[myPlayerId]->updateMovingState();
                }
                else sendInputPacket(ev.keyboard.keycode, isDown);
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            bool isDown = false;
            if (isServer && myPlayerId >= 0) {
                players[myPlayerId]->keyUP(ev.keyboard.keycode);
                players[myPlayerId]->updateMovingState();
            }
            else sendInputPacket(ev.keyboard.keycode, isDown);
        }
    }
    else if (currentState == STATE_PAUSE) {
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
}

void GameManager::resetPlayersToSpawn() {
    for (size_t i = 0; i < players.size(); i++) {
        players[i]->finished = false;
        if (i < baseMap.spawnPoints.size()) players[i]->setPos(baseMap.spawnPoints[i].x, baseMap.spawnPoints[i].y);
        else players[i]->setPos(100 + i * 32, 100);
    }
}

void GameManager::draw() {
    al_clear_to_color(al_map_rgb(0, 0, 0));

    if (currentState == STATE_MENU) {
        al_draw_text(font, al_map_rgb(255, 255, 0), SCREENWIDTH / 2, 50, ALLEGRO_ALIGN_CENTRE, "BACK 2 START");
        btnHost.draw(font, btnHost.isOver(gameMouseX, gameMouseY));
        btnJoin.draw(font, btnJoin.isOver(gameMouseX, gameMouseY));
        btnExit.draw(font, btnExit.isOver(gameMouseX, gameMouseY));
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, 275, ALLEGRO_ALIGN_CENTRE, inputIP.c_str());
    }
    else if (currentState == STATE_GAME || currentState == STATE_PAUSE) {
        if (baseMap.isLoaded) {
            baseMap.drawMap();
            baseMap.drawBus(doorClosed);
            for (auto& e : baseMap.entities) e->draw();
            for (auto& p : players) p->draw();
            if (fontSmall) al_draw_text(fontSmall, al_map_rgb(255, 255, 255), SCREENWIDTH - 10, 10, ALLEGRO_ALIGN_RIGHT, baseMap.title.c_str());
        }
        else {
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2, ALLEGRO_ALIGN_CENTRE, "LOADING...");
        }

        if (transitionAlpha > 0.0f) al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba_f(0, 0, 0, transitionAlpha));

        if (currentState == STATE_PAUSE) {
            al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba(0, 0, 0, 150));
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, 100, ALLEGRO_ALIGN_CENTRE, "PAUSADO");
            btnResume.draw(font, btnResume.isOver(gameMouseX, gameMouseY));
            btnQuit.draw(font, btnQuit.isOver(gameMouseX, gameMouseY));
        }
    }
    else if (currentState == STATE_ENDGAME) {
        al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, SCREENHEIGHT / 2, ALLEGRO_ALIGN_CENTRE, "FIM DE JOGO!");
    }
}

void GameManager::cleanup() {
    disconnectNetwork();

    for (auto p : players) {
        if (p) delete p;
    }
    players.clear();

    if (font) al_destroy_font(font);
    if (fontSmall) al_destroy_font(fontSmall);

    if (timer) al_destroy_timer(timer);
    if (queue) al_destroy_event_queue(queue);

    if (display) al_destroy_display(display);
}