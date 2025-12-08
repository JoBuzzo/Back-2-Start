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
      gameMouseX(0), gameMouseY(0),
      isDeathSequence(false), deathTimer(0.0f) // Inicializa variaveis de morte
{
    int btnW = 300; int btnH = 50;
    int centerX = SCREENWIDTH / 2 - (btnW / 2);
    btnHost = { centerX, 150, btnW, btnH, "CRIAR SERVIDOR" };
    btnJoin = { centerX, 350, btnW, btnH, "ENTRAR (CLIENTE)" };
    btnExit = { centerX, 450, btnW, btnH, "SAIR DO JOGO" };
    btnResume = { centerX, 200, btnW, btnH, "CONTINUAR" };
    btnQuit = { centerX, 300, btnW, btnH, "VOLTAR AO MENU" };

    // Lista de frases para a tela de morte
    mockeryList = {
        "ESCORREGOU NO QUIABO?",
        "FOI DE ARRASTA PRA CIMA.",
        "HABILIDADE COMPROMETIDA.",
        "NEM O GPS TE SALVA.",
        "TENTE NAO MORRER DA PROXIMA.",
        "LAG? ACHO QUE NAO...",
        "DE NOVO? SERIO?"
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

// --- NOVO MÉTODO: Inicia a Tela de Morte ---
void GameManager::startDeathSequence() {
    if (isDeathSequence) return;

    isDeathSequence = true;
    isTransitioning = true;
    doorClosed = true;
    transitionAlpha = 0.0f;
    deathTimer = 0.0f;

    // Escolhe frase
    int idx = rand() % mockeryList.size();
    currentMockery = mockeryList[idx];

    // Avisa Clientes
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

            // --- Logica de Transicao e Morte ---
            if (isTransitioning) {
                // Se for morte, escurece mais rápido
                transitionAlpha += isDeathSequence ? 0.05f : 0.02f;
                
                if (transitionAlpha >= 1.0f) {
                    transitionAlpha = 1.0f;

                    // === TELA DE MORTE ===
                    if (isDeathSequence) {
                        deathTimer += 1.0f / 60.0f; 

                        if (deathTimer >= 2.5f) { // Fica 2.5s na tela preta
                            
                            // Só o servidor reseta a lógica
                            if (net.isServer()) {
                                level.reset();         // Reseta o mapa (neve, carros)
                                resetPlayersToSpawn(); // Reseta posições
                            }
                            
                            // Fim da tela de morte
                            isDeathSequence = false;
                            isTransitioning = false; 
                            transitionAlpha = 0.0f; 
                            doorClosed = false;
                        }
                    }
                    // === TROCA DE FASE NORMAL ===
                    else if (net.isServer()) {
                        if (!level.nextLevelPath.empty()) {
                            level.loadMapFromJson(level.nextLevelPath);
                            resetPlayersToSpawn(); // Reseta (e zera gravidade)

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

                // Só processa lógica se não estiver na transição de morte
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
                            pkt->z,
                            pkt->isJumping
                        );
                }
                else if (type == PACKET_ENTITY_STATE) {
                    StatePacket* pkt = (StatePacket*)event.packet->data;
                    if (pkt->id >= 0 && pkt->id < level.entities.size()) {

                        Entity* e = level.entities[pkt->id];
                        e->posX = (int)pkt->x; 
                        e->posY = (int)pkt->y;
                        e->movingLeft = (pkt->current_frame_y == 1);
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
                    // --- NOVO: Cliente recebe Morte ---
                    else if (data[1] == MSG_DEATH_SEQUENCE) {
                        isDeathSequence = true;
                        isTransitioning = true;
                        transitionAlpha = 0.0f;
                        deathTimer = 0.0f;
                        
                        // Sorteia frase localmente
                        int idx = rand() % mockeryList.size();
                        currentMockery = mockeryList[idx];
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
    
    // 1. Atualiza Visual (Para todos)
    if (currentState == STATE_GAME && !isTransitioning) {
        level.updateWeather();
    }

    if (!net.isServer()) return;    

    int finishedCount = 0;
    int activePlayers = net.getNextId();
    bool accidentHappened = false;

    // 1. Movimento dos Players
    for (size_t i = 0; i < (size_t)activePlayers; i++) {
        
        int hx, hy, hw, hh;
        players[i]->getHitbox(hx, hy, hw, hh);
        
        int footX = hx + (hw / 2);
        int footY = hy + (hh / 2); 

        // --- CHECAR TILE ESPECIAL ---
        int tileID = level.getTileIdAt(footX, footY);
        TileData props = level.getTileProp(tileID);

        // Tile Mortal (Lava)
        if (props.deadly) {
            if (players[i]->z <= 2.0f) { 
                accidentHappened = true; // Marca reset coletivo
                break; // Sai do loop
            }
        }

        // Força
        if (props.forceX != 0 || props.forceY != 0) {
            players[i]->posX += (int)props.forceX;
            players[i]->posY += (int)props.forceY;
        }

        // Velocidade
        players[i]->terrainFactor = props.speedFactor; 

        // Movimento
        players[i]->move();
        players[i]->updateMovingState();

        // Checa Vitória
        players[i]->getHitbox(hx, hy, hw, hh);
        if (!players[i]->finished) {
            if (level.checkBusCollision(hx, hy, hw, hh))
                players[i]->finished = true;
        }
        else finishedCount++;
    }

    // 2. Colisão Entidades (Genérico)
    if (!accidentHappened) {
        for (auto& e : level.entities) {
            e->update(); 
            if (e->checkCollision(players)) { 
                accidentHappened = true;
            }
        }
    }

    // 3. Reset Coletivo (Agora com Tela de Morte)
    if (accidentHappened) {
        startDeathSequence();
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
    // Não precisa chamar level.reset() aqui, pois já chamamos no startDeathSequence
    // ou podemos manter se quiser garantir
    
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

void GameManager::handleInput(ALLEGRO_EVENT& ev) {
    if (ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
        gameMouseX = (ev.mouse.x - scaleX) / scale;
        gameMouseY = (ev.mouse.y - scaleY) / scale;
    }

    if (currentState == STATE_MENU) {
        if (ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
            if (btnHost.isOver(gameMouseX, gameMouseY)) {
                if (net.startHost(1234)) {
                    // Carrega do zero
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
                
                // RESET TOTAL AO SAIR
                level.reset(); // Limpa nevasca
                
                currentState = STATE_MENU;
            }
        }
    }
    else if (currentState == STATE_ENDGAME) {
        if (ev.type == ALLEGRO_EVENT_KEY_DOWN && ev.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            net.disconnect();
            for (auto p : players) delete p; players.clear();
            
            // RESET TOTAL AO SAIR
            level.reset(); 
            
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
            
            // 1. Mapa (Fundo)
            level.drawMap();
            
            // 2. Ônibus (Fundo estático)
            level.drawBus(doorClosed);

            // === LÓGICA DE Y-SORTING (SEM HERANÇA) ===
            
            // Estrutura auxiliar para guardar qualquer objeto desenhável
            struct RenderItem {
                float yVal; // Posição da BASE do objeto (Y + Altura)
                int type;   // 0 = Entity, 1 = Player
                void* ptr;  // Ponteiro genérico (aceita qualquer coisa)
            };

            std::vector<RenderItem> renderList;

            // -- Adiciona as ENTIDADES --
            for (auto& e : level.entities) {
                // Se a entidade não estiver ativa, nem adiciona na lista
                if(e->active) { 
                    renderList.push_back({ e->posY + e->h, 0, (void*)e });
                }
            }

            // -- Adiciona os PLAYERS --
            for (auto& p : players) {
                // Precisamos calcular a base do player (Pés)
                int hx, hy, hw, hh;
                p->getHitbox(hx, hy, hw, hh);
                
                // O Y de ordenação é o topo da hitbox + altura da hitbox (pés)
                float feetY = (float)(hy + hh); 
                
                renderList.push_back({ feetY, 1, (void*)p });
            }

            // -- ORDENAÇÃO --
            // Ordena do menor Y (fundo da tela) para o maior Y (frente da tela)
            std::sort(renderList.begin(), renderList.end(), [](const RenderItem& a, const RenderItem& b) {
                return a.yVal < b.yVal;
            });

            // -- DESENHO --
            for (auto& item : renderList) {
                if (item.type == 0) {
                    // É Entity: Faz cast e desenha
                    ((Entity*)item.ptr)->draw();
                } else {
                    // É Player: Faz cast e desenha
                    ((Player*)item.ptr)->draw();
                }
            }
            
            // 3. Overlay (Neve)
            level.drawWeather();
            
            // 4. UI
            if (fontSmall) al_draw_text(fontSmall, al_map_rgb(255, 255, 255), SCREENWIDTH - 10, 10, ALLEGRO_ALIGN_RIGHT, level.title.c_str());
        }
        else {
            al_draw_text(font, al_map_rgb(255, 255, 255), SCREENWIDTH / 2, SCREENHEIGHT / 2, ALLEGRO_ALIGN_CENTRE, "SINCRONIZANDO...");
        }

        if (transitionAlpha > 0.0f) 
            al_draw_filled_rectangle(0, 0, SCREENWIDTH, SCREENHEIGHT, al_map_rgba_f(0, 0, 0, transitionAlpha));

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
       // ... (Código endgame igual) ...
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

    for (int i = 0; i < level.entities.size(); i++) {
        Entity* e = level.entities[i];
        StatePacket pkt;
        pkt.type = PACKET_ENTITY_STATE;
        pkt.id = i;
        pkt.x = e->posX;
        pkt.y = e->posY;
        pkt.current_frame_y = e->movingLeft ? 1 : 0;
        pkt.isMoving = true;
        pkt.isFinished = false;

        net.broadcastPacket(&pkt, sizeof(StatePacket), false);
    }
}