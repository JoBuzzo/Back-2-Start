#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "src/managers/game/GameManager.h"
#include "src/managers/resource/ResourceManager.h"
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <cmath>

// Include dos players
#include "src/entities/players/Buzzo/Buzzo.h"
#include "src/entities/players/Emanuel/Emanuel.h"
#include "src/entities/players/Galvao/Galvao.h"
#include "src/entities/players/Luis/Luis.h"
#include "src/entities/players/Renatinho/Renatinho.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

// --- Métodos Auxiliares de UI ---
bool Button::isOver(float mx, float my) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void Button::draw(ALLEGRO_FONT* font, bool hover) {
    al_draw_filled_rectangle(x, y, x + w, y + h, hover ? al_map_rgb(100, 100, 255) : al_map_rgb(50, 50, 50));
    al_draw_rectangle(x, y, x + w, y + h, al_map_rgb(255, 255, 255), 2);
    if(font) al_draw_text(font, al_map_rgb(255, 255, 255), x + w / 2, y + 10, ALLEGRO_ALIGN_CENTRE, text);
}

// --- GameManager Implementation ---

GameManager::GameManager() 
    : display(nullptr), queue(nullptr), timer(nullptr), font(nullptr), fontSmall(nullptr), trans(),
      running(true), redraw(true), currentState(STATE_MENU), inputIP("127.0.0.1"),
      scale(1.0f), scaleX(0.0f), scaleY(0.0f),
      isTransitioning(false), transitionAlpha(0.0f), doorClosed(false),
      gameMouseX(0), gameMouseY(0),
      isDeathSequence(false), deathTimer(0.0f) 
{
    srand(static_cast<unsigned int>(time(0)));

    int btnW = 300; int btnH = 50;
    int centerX = SCREENWIDTH / 2 - (btnW / 2);
    btnHost = { centerX, 150, btnW, btnH, "CRIAR SERVIDOR" };
    btnJoin = { centerX, 350, btnW, btnH, "ENTRAR (CLIENTE)" };
    btnExit = { centerX, 450, btnW, btnH, "SAIR DO JOGO" };
    btnResume = { centerX, 200, btnW, btnH, "CONTINUAR" };
    btnQuit = { centerX, 300, btnW, btnH, "VOLTAR AO MENU" };

    mockeryList = {
        "ESCORREGOU NO QUIABO?", "FOI DE ARRASTA PRA CIMA.", "HABILIDADE COMPROMETIDA.",
        "NEM O GPS TE SALVA.", "TENTE NAO MORRER DA PROXIMA.", "LAG? ACHO QUE NAO...", "DE NOVO? SERIO?"
    };
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

void GameManager::startDeathSequence() {
    if (isDeathSequence) return;
    isDeathSequence = true; isTransitioning = true;
    doorClosed = true; transitionAlpha = 0.0f; deathTimer = 0.0f;
    int idx = rand() % mockeryList.size();
    currentMockery = mockeryList[idx];

    if (net.isServer()) {
        int data[2] = { PACKET_CHANGE_LEVEL, MSG_DEATH_SEQUENCE };
        net.broadcastPacket(data, sizeof(data), true);
    }
}

void GameManager::run() {
    if (!net.init()) return;
    if (!initAllegro()) return;

    while (running) {
        ALLEGRO_EVENT ev;
        al_wait_for_event(queue, &ev);

        if (ev.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;

            // --- Transição / Morte ---
            if (isTransitioning) {
                transitionAlpha += isDeathSequence ? 0.05f : 0.02f;
                if (transitionAlpha >= 1.0f) {
                    transitionAlpha = 1.0f;
                    if (isDeathSequence) {
                        deathTimer += 1.0f / 60.0f; 
                        if (deathTimer >= 2.5f) { 
                            if (net.isServer()) { level.reset(); resetPlayersToSpawn(); }
                            isDeathSequence = false; isTransitioning = false; 
                            transitionAlpha = 0.0f; doorClosed = false;
                        }
                    }
                    else if (net.isServer()) {
                        if (!level.nextLevelPath.empty()) {
                            level.loadMapFromJson(level.nextLevelPath);
                            resetPlayersToSpawn(); 
                            int data[2] = { PACKET_CHANGE_LEVEL, MSG_LOAD_MAP };
                            net.broadcastPacket(data, sizeof(data), true);
                            isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                        } else {
                            int data[2] = { PACKET_CHANGE_LEVEL, MSG_END_GAME };
                            net.broadcastPacket(data, sizeof(data), true);
                            net.flush();
                            currentState = STATE_ENDGAME; isTransitioning = false;
                        }
                    }
                }
            }

            // --- Update Geral ---
            if (currentState == STATE_GAME || currentState == STATE_PAUSE || currentState == STATE_ENDGAME) {
                processNetwork();
                if (currentState == STATE_GAME && !isTransitioning) {
                    updateGameLogic();
                }
            }
        }
        else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) running = false;
        else handleInput(ev);

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false; draw(); al_flip_display();
        }
    }
}

void GameManager::processNetwork() {
    net.update(
        [&](ENetPeer* peer) { // Connect
            if (net.isServer() && net.getNextId() < players.size()) {
                WelcomePacket wpkt; wpkt.assignedId = net.getNextId();
                std::string map = level.path.empty() ? "assets/maps/level1.json" : level.path;
                strncpy_s(wpkt.currentLevelPath, sizeof(wpkt.currentLevelPath), map.c_str(), 127);
                net.sendPacket(peer, &wpkt, sizeof(WelcomePacket), true);
                net.incrementNextId();
            }
        },
        [&](ENetPeer* peer) { printf("Jogador desconectou.\n"); }, // Disconnect
        [&](ENetEvent& event) { // Receive
            if (event.packet->dataLength < sizeof(int)) return;
            int type = *(int*)event.packet->data;

            if (!net.isServer()) { // CLIENTE
                if (type == PACKET_WELCOME) {
                    WelcomePacket* pkt = (WelcomePacket*)event.packet->data;
                    net.setMyId(pkt->assignedId);
                    level.loadMapFromJson(pkt->currentLevelPath);
                }
                else if (type == PACKET_STATE) {
                    StatePacket* pkt = (StatePacket*)event.packet->data;
                    if (pkt->id >= 0 && pkt->id < players.size()) {
                        Player* p = players[pkt->id];

                        // Detecção de movimento visual para forçar 'isMoving'
                        bool visualMove = (abs(p->posX - (int)pkt->x) > 0 || abs(p->posY - (int)pkt->y) > 0);
                        bool forceAnimate = visualMove || pkt->isMoving;

                        p->setNetworkState(
                            pkt->x, pkt->y, pkt->current_frame_y, 
                            forceAnimate, 
                            pkt->isRunning,
                            pkt->isFinished, pkt->z, pkt->isJumping
                        );

                        p->updateSpriteSheet(); 
                    }
                }
                else if (type == PACKET_ENTITY_STATE) {
                    StatePacket* pkt = (StatePacket*)event.packet->data;
                    if (pkt->id >= 0 && pkt->id < level.entities.size()) {
                        Entity* e = level.entities[pkt->id];
                        e->posX = (int)pkt->x; e->posY = (int)pkt->y;
                        e->movingLeft = (pkt->current_frame_y == 1);
                    }
                }
                else if (type == PACKET_START_TRANSITION) {
                    isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
                }
                else if (type == PACKET_CHANGE_LEVEL) {
                    int* data = (int*)event.packet->data;
                    if (data[1] == MSG_LOAD_MAP) {
                        level.loadMapFromJson(level.nextLevelPath);
                        isTransitioning = false; doorClosed = false; transitionAlpha = 0.0f;
                    }
                    else if (data[1] == MSG_END_GAME) {
                        currentState = STATE_ENDGAME; isTransitioning = false; transitionAlpha = 0.0f;
                    }
                    else if (data[1] == MSG_DEATH_SEQUENCE) {
                        isDeathSequence = true; isTransitioning = true; transitionAlpha = 0.0f; deathTimer = 0.0f;
                        currentMockery = mockeryList[rand() % mockeryList.size()];
                    }
                }
            }
            else if (type == PACKET_INPUT) { // SERVIDOR
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

    // CLIENTE: Só anima entidades
    if (!net.isServer()) {
        for(auto& e : level.entities) e->update(); 
        return; 
    }

    // SERVIDOR: Física e Colisão
    int finishedCount = 0;
    int activePlayers = net.getNextId();
    bool accidentHappened = false;

    for (size_t i = 0; i < (size_t)activePlayers; i++) {
        int hx, hy, hw, hh;
        players[i]->getHitbox(hx, hy, hw, hh);
        
        int tileID = level.getTileIdAt(hx + hw/2, hy + hh/2);
        TileData props = level.getTileProp(tileID);

        if (props.deadly && players[i]->z <= 2.0f) { accidentHappened = true; break; }
        if (props.forceX != 0 || props.forceY != 0) {
            players[i]->posX += (int)props.forceX; players[i]->posY += (int)props.forceY;
        }
        players[i]->terrainFactor = props.speedFactor; 
        
        players[i]->move(); 
        players[i]->getHitbox(hx, hy, hw, hh);
        
        if (!players[i]->finished) {
            if (level.checkBusCollision(hx, hy, hw, hh)) players[i]->finished = true;
        } else finishedCount++;
    }

    if (!accidentHappened) {
        for (auto& e : level.entities) {
            e->update(); 
            if (e->checkCollision(players)) accidentHappened = true;
        }
    }

    if (accidentHappened) startDeathSequence();

    if (finishedCount > 0 && finishedCount == activePlayers) {
        isTransitioning = true; doorClosed = true; transitionAlpha = 0.0f;
        int type = PACKET_START_TRANSITION;
        net.broadcastPacket(&type, sizeof(int), true);
    }

    broadcastState();
}

// Struct Y-Sorting
struct RenderItem {
    float yVal; 
    int type; // 0 = Entity, 1 = Player
    void* ptr;
};

void GameManager::draw() {
    al_clear_to_color(al_map_rgb(0, 0, 0));

    if (currentState == STATE_MENU) {
        al_draw_text(font, al_map_rgb(255, 255, 0), SCREENWIDTH / 2, 50, ALLEGRO_ALIGN_CENTRE, "BACK 2 START");
        btnHost.draw(font, btnHost.isOver(gameMouseX, gameMouseY));
        btnJoin.draw(font, btnJoin.isOver(gameMouseX, gameMouseY));
        btnExit.draw(font, btnExit.isOver(gameMouseX, gameMouseY));
        al_draw_text(font, al_map_rgb(200, 200, 200), SCREENWIDTH / 2, 240, ALLEGRO_ALIGN_CENTRE, "Digite o IP:");
        al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, 275, ALLEGRO_ALIGN_CENTRE, inputIP.c_str());
    }
    else if (currentState == STATE_GAME || currentState == STATE_PAUSE) {
        if (level.isLoaded) {
            level.drawMap();
            level.drawBus(doorClosed);

            // --- Y-SORTING ---
            std::vector<RenderItem> renderList;
            for (auto& e : level.entities) if(e->active) renderList.push_back({ e->posY + e->h, 0, (void*)e });
            for (auto& p : players) {
                int hx, hy, hw, hh; p->getHitbox(hx, hy, hw, hh);
                renderList.push_back({ (float)(hy + hh), 1, (void*)p });
            }
            std::sort(renderList.begin(), renderList.end(), [](const RenderItem& a, const RenderItem& b) {
                return a.yVal < b.yVal;
            });
            for (auto& item : renderList) {
                if (item.type == 0) ((Entity*)item.ptr)->draw();
                else ((Player*)item.ptr)->draw();
            }
            
            level.drawWeather();
            if (fontSmall) al_draw_text(fontSmall, al_map_rgb(255, 255, 255), SCREENWIDTH - 10, 10, ALLEGRO_ALIGN_RIGHT, level.title.c_str());
        }
        else al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2, ALLEGRO_ALIGN_CENTRE, "SINCRONIZANDO...");

        if (transitionAlpha > 0.0f) al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba_f(0, 0, 0, transitionAlpha));
        if (isDeathSequence && transitionAlpha > 0.9f) {
            al_draw_text(font, al_map_rgb(255, 50, 50), SCREENWIDTH / 2, SCREENHEIGHT / 2 - 20, ALLEGRO_ALIGN_CENTRE, "VOCE PERDEU!");
            al_draw_text(fontSmall, al_map_rgb(200, 200, 200), SCREENWIDTH / 2, SCREENHEIGHT / 2 + 20, ALLEGRO_ALIGN_CENTRE, currentMockery.c_str());
        }
        if (currentState == STATE_PAUSE) {
            al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba(0, 0, 0, 150));
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, 100, ALLEGRO_ALIGN_CENTRE, "PAUSADO");
            btnResume.draw(font, btnResume.isOver(gameMouseX, gameMouseY));
            btnQuit.draw(font, btnQuit.isOver(gameMouseX, gameMouseY));
        }
    }
    else if (currentState == STATE_ENDGAME) {
       al_draw_text(font, al_map_rgb(0, 255, 0), SCREENWIDTH / 2, SCREENHEIGHT / 2 - 20, ALLEGRO_ALIGN_CENTRE, "PARABENS!");
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
        
        pkt.isRunning = players[i]->isRunning;
        
        pkt.isFinished = players[i]->finished;
        pkt.z = players[i]->z;
        pkt.isJumping = players[i]->isJumping;
        net.broadcastPacket(&pkt, sizeof(StatePacket), false);
    }
    for (int i = 0; i < level.entities.size(); i++) {
        Entity* e = level.entities[i];
        StatePacket pkt; pkt.type = PACKET_ENTITY_STATE; pkt.id = i;
        pkt.x = e->posX; pkt.y = e->posY;
        pkt.current_frame_y = e->movingLeft ? 1 : 0;
        net.broadcastPacket(&pkt, sizeof(StatePacket), false);
    }
}

// --- Implementação de resetPlayersToSpawn ---
void GameManager::resetPlayersToSpawn() {
    for (size_t i = 0; i < players.size(); i++) {
        players[i]->finished = false;
        
        // Zera física
        players[i]->z = 0;
        players[i]->vz = 0;
        players[i]->isJumping = false;

        if (i < level.spawnPoints.size()) 
            players[i]->setPos(level.spawnPoints[i].x, level.spawnPoints[i].y);
        else 
            players[i]->setPos(100 + i * 32, 100);
    }
}

// --- Implementação de handleInput ---
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
                    players.push_back(new Renatinho()); 
                    players.push_back(new Luis());
                    players.push_back(new Emanuel());
                    players.push_back(new Galvao());

                    resetPlayersToSpawn();
                    currentState = STATE_GAME;
                }
            }
            else if (btnJoin.isOver(gameMouseX, gameMouseY)) {
                if (net.startClient(inputIP, 1234)) {
                    players.clear();
                    players.push_back(new Buzzo()); 
                    players.push_back(new Renatinho());
                    players.push_back(new Luis());
                    players.push_back(new Emanuel());
                    players.push_back(new Galvao());
                    
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
                } else {
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
            } else {
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
                
                level.reset();
                
                currentState = STATE_MENU;
            }
        }
    }
    else if (currentState == STATE_ENDGAME) {
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            net.disconnect();
            for (auto p : players) delete p; players.clear();
            
            level.reset(); 
            
            currentState = STATE_MENU;
        }
    }
}