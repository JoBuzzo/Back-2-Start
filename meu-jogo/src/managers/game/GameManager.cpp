#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "src/managers/game/GameManager.h"
#include "src/managers/resource/ResourceManager.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <cstdio>

// Include dos bichos
#include "src/entities/players/Bull/Bull.h"
#include "src/entities/players/Buzzo/Buzzo.h"
#include "src/entities/players/Chicken/Chicken.h"
#include "src/entities/players/Pig/Pig.h"
#include "src/entities/players/Sheep/Sheep.h"
#include "src/entities/players/Turkey/Turkey.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// --- Métodos Auxiliares de UI ---
bool Button::isOver(float mx, float my) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void Button::draw(ALLEGRO_FONT* font, bool hover) {
    al_draw_filled_rectangle(x, y, x + w, y + h, hover ? al_map_rgb(100, 100, 255) : al_map_rgb(50, 50, 50));
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255, 255, 255), 2);
    al_draw_text(font, al_map_rgb(255, 255, 255), x + w / 2, y + 10, ALLEGRO_ALIGN_CENTRE, text);
}

// --- GameManager Implementation ---

GameManager::GameManager() 
    : display(nullptr), queue(nullptr), timer(nullptr), font(nullptr), fontSmall(nullptr), trans(),
      running(true), redraw(true), currentState(STATE_MENU), inputIP("127.0.0.1"),
      scale(1.0f), scaleX(0.0f), scaleY(0.0f),
      isTransitioning(false), transitionAlpha(0.0f), doorClosed(false),
      gameMouseX(0), gameMouseY(0)
{
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

    int monitorW = al_get_display_width(display);
    int monitorH = al_get_display_height(display);
    float sx = monitorW / (float)SCREENWIDTH;
    float sy = monitorH / (float)SCREENHEIGHT;
    scale = (sx < sy) ? sx : sy;
    scaleX = (monitorW - (SCREENWIDTH * scale)) / 2;
    scaleY = (monitorH - (SCREENHEIGHT * scale)) / 2;

    al_identity_transform(&trans);
    al_scale_transform(&trans, scale, scale);
    al_translate_transform(&trans, scaleX, scaleY);
    al_use_transform(&trans);

    font = ResourceManager::get().getFont("assets/fonts/font.ttf", 25);
    fontSmall = ResourceManager::get().getFont("assets/fonts/font.ttf", 18);

    timer = al_create_timer(1.0 / 60.0);
    queue = al_create_event_queue();

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_start_timer(timer);

    return true;
}

void GameManager::cleanup() {
    net.shutdown();

    for (auto p : players) if (p) delete p;
    players.clear();

    if (timer) al_destroy_timer(timer);
    if (queue) al_destroy_event_queue(queue);

    ResourceManager::get().clear();

    if (display) al_destroy_display(display);
}

void GameManager::run() {
    if (!net.init()) return;
    if (!initAllegro()) return;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;

            // --- Logica de Transicao ---
            if (isTransitioning) {
                transitionAlpha += 0.02f;
                if (transitionAlpha >= 1.0f) {
                    transitionAlpha = 1.0f;
                    if (net.isServer()) {
                        if (!level.nextLevelPath.empty()) {
                            level.loadMapFromJson(level.nextLevelPath);
                            resetPlayersToSpawn();

                            int data[2] = { PACKET_CHANGE_LEVEL, MSG_LOAD_MAP };
                            net.broadcastPacket(data, sizeof(data), true);

                            isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                        }
                        else {
                            int data[2] = { PACKET_CHANGE_LEVEL, MSG_END_GAME };
                            net.broadcastPacket(data, sizeof(data), true);
                            net.flush();
                            currentState = STATE_ENDGAME; isTransitioning = false;
                        }
                    }
                }
            }

            // --- Loop de Jogo ---
            if (currentState == STATE_GAME || currentState == STATE_PAUSE || currentState == STATE_ENDGAME) {
                processNetwork();

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

void GameManager::processNetwork() {
    net.update(
        // 1. On Connect
        [&](ENetPeer* peer) {
            if (net.isServer()) {
                if (net.getNextId() < players.size()) {
                    WelcomePacket wpkt;
                    wpkt.assignedId = net.getNextId();

                    std::string currentMap = level.path.empty() ? "assets/maps/level1.json" : level.path;
                    strncpy_s(wpkt.currentLevelPath, sizeof(wpkt.currentLevelPath), currentMap.c_str(), 127);

                    net.sendPacket(peer, &wpkt, sizeof(WelcomePacket), true);
                    net.incrementNextId();
                }
            }
        },

        // 2. On Disconnect
        [&](ENetPeer* peer) {
            printf("Jogador desconectou.\n");
        },

        // 3. On Receive
        [&](ENetEvent& event) {
            if (event.packet->dataLength < sizeof(int)) return;
            int type = *(int*)event.packet->data;

            // CLIENTE
            if (!net.isServer()) {
                if (type == PACKET_WELCOME) {
                    WelcomePacket* pkt = (WelcomePacket*)event.packet->data;
                    net.setMyId(pkt->assignedId);
                    level.loadMapFromJson(pkt->currentLevelPath);
                }
                else if (type == PACKET_STATE) {
                    StatePacket* pkt = (StatePacket*)event.packet->data;
                    if (pkt->id >= 0 && pkt->id < players.size())
                        players[pkt->id]->setNetworkState(
                            pkt->x, 
                            pkt->y, 
                            pkt->current_frame_y, 
                            pkt->isMoving, 
                            pkt->isFinished,
                            pkt->z,         // <--- AGORA RECEBE O Z (Sombra)
                            pkt->isJumping  // <--- AGORA RECEBE O PULO
                        );
                }
                else if (type == PACKET_ENTITY_STATE) {
                    StatePacket* pkt = (StatePacket*)event.packet->data;
                    if (pkt->id >= 0 && pkt->id < level.entities.size()) {
                        Car* car = (Car*)level.entities[pkt->id];
                        car->posX = (int)pkt->x; car->posY = (int)pkt->y;
                        car->movingLeft = (pkt->current_frame_y == 1);
                    }
                }
                else if (type == PACKET_START_TRANSITION) {
                    isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
                }
                else if (type == PACKET_CHANGE_LEVEL && event.packet->dataLength >= sizeof(int) * 2) {
                    int* data = (int*)event.packet->data;
                    if (data[1] == MSG_LOAD_MAP) {
                        if (!level.nextLevelPath.empty()) {
                            level.loadMapFromJson(level.nextLevelPath);
                            isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                        }
                    }
                    else if (data[1] == MSG_END_GAME) {
                        currentState = STATE_ENDGAME;
                        isTransitioning = false; transitionAlpha = 0.0f;
                    }
                }
            }
            // SERVIDOR
            else if (type == PACKET_INPUT) {
                InputPacket* pkt = (InputPacket*)event.packet->data;
                if (pkt->playerId >= 0 && pkt->playerId < players.size()) {
                    if (pkt->isDown) players[pkt->playerId]->keyDOWN(pkt->keycode);
                    else players[pkt->playerId]->keyUP(pkt->keycode);
                    players[pkt->playerId]->updateMovingState();
                }
            }
        }
    );
}

void GameManager::updateGameLogic() {
    
    if (currentState == STATE_GAME && !isTransitioning) {
        level.updateWeather();
    }

    if (!net.isServer()) return;


    int finishedCount = 0;
    int activePlayers = net.getNextId();
    bool accidentHappened = false;

    // 1. Movimento dos Players
    for (size_t i = 0; i < (size_t)activePlayers; i++) {
        
        // --- A. PEGAR HITBOX ATUAL ---
        int hx, hy, hw, hh;
        players[i]->getHitbox(hx, hy, hw, hh);
        
        int footX = hx + (hw / 2);
        int footY = hy + (hh / 2); 

        // --- CHECAR TILE ESPECIAL ---
        int tileID = level.getTileIdAt(footX, footY);
        TileData props = level.getTileProp(tileID);

        // Tile Mortal (Lava/Espinhos/Buraco)
        if (props.deadly) {
            // Só morre se estiver no chão (z baixo)
            if (players[i]->z <= 2.0f) { 
                accidentHappened = true;
                break;
            }
        }

        // Tile de Força (Correnteza/Esteira)
        if (props.forceX != 0 || props.forceY != 0) {
            players[i]->posX += (int)props.forceX;
            players[i]->posY += (int)props.forceY;
        }

        // Tile de Velocidade (Lama/Gelo)
        players[i]->terrainFactor = props.speedFactor; 

        // --- MOVIMENTO NORMAL ---
        players[i]->move();
        players[i]->updateMovingState();

        // Recalcula hitbox após movimento para colisão do ônibus
        players[i]->getHitbox(hx, hy, hw, hh);

        if (!players[i]->finished) {
            if (level.checkBusCollision(hx, hy, hw, hh))
                players[i]->finished = true;
        }
        else finishedCount++;
    }

    // 2. Movimento e Colisao dos Carros
    if (!accidentHappened) {
        for (auto& e : level.entities) {
            e->move();
            if (e->checkCollision(players)) { 
                accidentHappened = true;
            }
        }
    }

    // 3. Reset Coletivo (Lava ou Carro)
    if (accidentHappened) {
        resetPlayersToSpawn();
    }

    // 4. Vitoria
    if (finishedCount > 0 && finishedCount == activePlayers) {
        isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
        int type = PACKET_START_TRANSITION;
        net.broadcastPacket(&type, sizeof(int), true);
    }

    broadcastState();
}

void GameManager::resetPlayersToSpawn() {
    for (size_t i = 0; i < players.size(); i++) {
        players[i]->finished = false;
        
        // --- CORREÇÃO: Zera a física do pulo ---
        players[i]->z = 0;
        players[i]->vz = 0;
        players[i]->isJumping = false;
        // ---------------------------------------

        if (i < level.spawnPoints.size()) 
            players[i]->setPos(level.spawnPoints[i].x, level.spawnPoints[i].y);
        else 
            players[i]->setPos(100 + i * 32, 100);
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
                if (net.startHost(1234)) {
                    level.loadMapFromJson("assets/maps/level1.json");
                    players.clear();
                    players.push_back(new Buzzo()); 
                    players.push_back(new Chicken()); 
                    players.push_back(new Turkey());
                    players.push_back(new Bull());
                    players.push_back(new Sheep()); 
                    players.push_back(new Pig()); 

                    resetPlayersToSpawn();
                    currentState = STATE_GAME;
                }
            }
            else if (btnJoin.isOver(gameMouseX, gameMouseY)) {
                if (net.startClient(inputIP, 1234)) {
                    players.clear();
                    players.push_back(new Buzzo()); 
                    players.push_back(new Chicken()); 
                    players.push_back(new Turkey());
                    players.push_back(new Bull());
                    players.push_back(new Sheep()); 
                    players.push_back(new Pig()); 
                    
                    currentState = STATE_GAME;
                }
            }
            else if (btnExit.isOver(gameMouseX, gameMouseY)) running = false;
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
                if (net.isServer() && net.getMyId() >= 0) {
                    players[net.getMyId()]->keyDOWN(ev.keyboard.keycode);
                    players[net.getMyId()]->updateMovingState();
                }
                else {
                    InputPacket pkt; pkt.playerId = net.getMyId(); pkt.keycode = ev.keyboard.keycode; pkt.isDown = isDown;
                    net.sendPacket(nullptr, &pkt, sizeof(InputPacket), true);
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
            bool isDown = false;
            if (net.isServer() && net.getMyId() >= 0) {
                players[net.getMyId()]->keyUP(ev.keyboard.keycode);
                players[net.getMyId()]->updateMovingState();
            }
            else {
                InputPacket pkt; pkt.playerId = net.getMyId(); pkt.keycode = ev.keyboard.keycode; pkt.isDown = isDown;
                net.sendPacket(nullptr, &pkt, sizeof(InputPacket), true);
            }
        }
    }
    else if (currentState == STATE_PAUSE) {
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (btnResume.isOver(gameMouseX, gameMouseY)) currentState = STATE_GAME;
            else if (btnQuit.isOver(gameMouseX, gameMouseY)) {
                net.disconnect();
                for (auto p : players) delete p; players.clear();
                currentState = STATE_MENU;
            }
        }
    }
    else if (currentState == STATE_ENDGAME) {
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            net.disconnect();
            for (auto p : players) delete p; players.clear();
            currentState = STATE_MENU;
        }
    }
}

void GameManager::draw() {
    al_clear_to_color(al_map_rgb(0, 0, 0));

    if (currentState == STATE_MENU) {
        al_draw_text(font, al_map_rgb(255, 255, 0), SCREENWIDTH / 2, 50, ALLEGRO_ALIGN_CENTRE, "BACK 2 START");
        btnHost.draw(font, btnHost.isOver(gameMouseX, gameMouseY));
        btnJoin.draw(font, btnJoin.isOver(gameMouseX, gameMouseY));
        btnExit.draw(font, btnExit.isOver(gameMouseX, gameMouseY));
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREENWIDTH / 2, 240, ALLEGRO_ALIGN_CENTRE, "Digite o IP do Host:");
        al_draw_rectangle(SCREENWIDTH / 2 - 150, 270, SCREENWIDTH / 2 + 150, 310, al_map_rgb(255, 255, 255), 2);
        al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, 275, ALLEGRO_ALIGN_CENTRE, inputIP.c_str());
    }
    else if (currentState == STATE_GAME || currentState == STATE_PAUSE) {
        if (level.isLoaded) {
            
            level.drawMap();
            level.drawBus(doorClosed);
            for (auto& e : level.entities) e->draw();
            for (auto& p : players) p->draw();
            
            level.drawWeather();
            if (fontSmall) al_draw_text(fontSmall, al_map_rgb(255, 255, 255), SCREENWIDTH - 10, 10, ALLEGRO_ALIGN_RIGHT, level.title.c_str());
        }
        else {
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2, ALLEGRO_ALIGN_CENTRE, "SINCRONIZANDO...");
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
        al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, SCREENHEIGHT / 2 - 20, ALLEGRO_ALIGN_CENTRE, "PARABENS!");
        al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2 + 20, ALLEGRO_ALIGN_CENTRE, "VOCES COMPLETARAM O JOGO.");
        al_draw_text(font, al_map_rgb(100, 100, 100), SCREENWIDTH / 2, SCREENHEIGHT - 50, ALLEGRO_ALIGN_CENTRE, "Pressione ESC para voltar");
    }
}

void GameManager::broadcastState() {
    for (int i = 0; i < players.size(); i++) {
        StatePacket pkt;
        pkt.type = PACKET_STATE;
        pkt.id = i;
        pkt.x = players[i]->posX;
        pkt.y = players[i]->posY;
        pkt.current_frame_y = players[i]->current_frame_y;
        pkt.isMoving = players[i]->isMoving;
        pkt.isFinished = players[i]->finished;

        pkt.z = players[i]->z;
        pkt.isJumping = players[i]->isJumping;

        net.broadcastPacket(&pkt, sizeof(StatePacket), false);
    }

    // Envia estado dos Carros
    for (int i = 0; i < level.entities.size(); i++) {
        Car* car = (Car*)level.entities[i];
        StatePacket pkt;
        pkt.type = PACKET_ENTITY_STATE;
        pkt.id = i;
        pkt.x = car->posX;
        pkt.y = car->posY;
        pkt.current_frame_y = car->movingLeft ? 1 : 0;
        pkt.isMoving = true;
        pkt.isFinished = false;

        net.broadcastPacket(&pkt, sizeof(StatePacket), false);
    }
}