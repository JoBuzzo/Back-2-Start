#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Config.h"
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>
#include <enet/enet.h>
#include "NetworkProtocol.h"

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#include <vector>
#include <iostream>
#include "Car.h"
#include "Player.h"
#include "BaseMap.h"
#include "Bull.h"
#include "Chicken.h"
#include "Pig.h"
#include "Sheep.h"
#include "Turkey.h"

ENetHost* netHost = nullptr;
ENetPeer* netPeer = nullptr;
bool isServer = false;
int myPlayerId = 0;

void initNetwork() {
    if (enet_initialize() != 0) {
        printf("Erro fatal: Nao foi possivel iniciar ENet.\n");
        exit(1);
    }
    atexit(enet_deinitialize);

    printf("MODO DE JOGO:\n[1] CRIAR Servidor (Host)\n[2] ENTRAR como Cliente\nEscolha: ");
    int choice;
    std::cin >> choice;

    if (choice == 1) {
        isServer = true;
        myPlayerId = 0;

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = 1234;

        netHost = enet_host_create(&address, 32, 2, 0, 0);
        if (netHost == NULL) {
            printf("Erro ao criar host servidor.\n");
            exit(1);
        }
        printf("Servidor criado na porta 1234. Aguardando jogadores...\n");
    }
    else {
        isServer = false;
        myPlayerId = 1;

        netHost = enet_host_create(NULL, 1, 2, 0, 0);

        ENetAddress address;
        printf("Digite o IP do servidor (ex: 127.0.0.1): ");
        char ip[50];
        std::cin >> ip;

        enet_address_set_host(&address, ip);
        address.port = 1234;

        printf("Tentando conectar...\n");
        netPeer = enet_host_connect(netHost, &address, 2, 0);

        if (netPeer == NULL) {
            printf("Nao foi possivel iniciar conexao.\n");
            exit(1);
        }

        ENetEvent event;
        if (enet_host_service(netHost, &event, 5000) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT) {
            printf("Conectado ao servidor com sucesso!\n");
        }
        else {
            printf("Falha na conexao (Time out).\n");
            system("pause");
            exit(1);
        }
    }
}

void sendInputPacket(int keycode, bool isDown) {
    InputPacket pkt;
    pkt.playerId = myPlayerId;
    pkt.keycode = keycode;
    pkt.isDown = isDown;

    ENetPacket* packet = enet_packet_create(&pkt, sizeof(InputPacket), ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(netPeer, 0, packet);
}

int main() {
    initNetwork();

    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();

    ALLEGRO_DISPLAY* display = al_create_display(SCREENWIDTH, SCREENHEIGHT);
    al_set_window_title(display, isServer ? "Street Tile! (SERVIDOR)" : "Street Tile! (CLIENTE)");

    ALLEGRO_MONITOR_INFO info;
    al_get_monitor_info(0, &info);
    int displayWidth = info.x2 - info.x1;
    int displayHeight = info.y2 - info.y1;
    int posX = (displayWidth - SCREENWIDTH) / 2;
    int posY = (displayHeight - SCREENHEIGHT) / 2;
    al_set_window_position(display, posX, posY);

    ALLEGRO_FONT* font = al_load_font("assets/fonts/font.ttf", 25, 0);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    BaseMap baseMap;
    if (!baseMap.loadMapFromJson("assets/maps/level1.json")) {
        printf("Erro ao carregar mapa. Verifique a pasta assets.\n");
        system("pause");
        return -1;
    }

    std::vector<Player*> players;
    players.push_back(new Chicken()); // ID 0
    players.push_back(new Bull());    // ID 1
    players.push_back(new Pig());     // ID 2
    players.push_back(new Sheep());   // ID 3
    players.push_back(new Turkey());  // ID 4

    bool redraw = true;

    while (true) {

        ENetEvent netEvent;
        while (enet_host_service(netHost, &netEvent, 0) > 0) {
            switch (netEvent.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("Um novo jogador conectou!\n");
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (netEvent.packet->dataLength >= sizeof(int)) {
                    int type = *(int*)netEvent.packet->data;

                    if (isServer && type == PACKET_INPUT) {
                        InputPacket* pkt = (InputPacket*)netEvent.packet->data;
                        if (pkt->playerId >= 0 && pkt->playerId < players.size()) {
                            if (pkt->isDown) players[pkt->playerId]->keyDOWN(pkt->keycode);
                            else players[pkt->playerId]->keyUP(pkt->keycode);

                            players[pkt->playerId]->updateMovingState();
                        }
                    }
                    else if (!isServer) {
                        if (type == PACKET_STATE) {
                            StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                            if (pkt->id >= 0 && pkt->id < players.size()) {
                                players[pkt->id]->setNetworkState(
                                    pkt->x,
                                    pkt->y,
                                    pkt->current_frame_y,
                                    pkt->isMoving
                                );
                            }
                        }
                        else if (type == PACKET_ENTITY_STATE) {
                            StatePacket* pkt = (StatePacket*)netEvent.packet->data;
                            if (pkt->id >= 0 && pkt->id < baseMap.entities.size()) {
                                Car* car = (Car*)baseMap.entities[pkt->id];
                                car->posX = (int)pkt->x;
                                car->posY = (int)pkt->y;
                                car->movingLeft = (pkt->current_frame_y == 1);
                            }
                        }
                    }
                }
                enet_packet_destroy(netEvent.packet);
                break;
            }
        }

        ALLEGRO_EVENT ev;
        while (al_get_next_event(queue, &ev)) {
            if (ev.type == ALLEGRO_EVENT_TIMER) {
                redraw = true;
            }
            else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                goto cleanup;
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_DOWN || ev.type == ALLEGRO_EVENT_KEY_UP) {
                bool isDown = (ev.type == ALLEGRO_EVENT_KEY_DOWN);

                if (isServer) {
                    if (isDown) players[myPlayerId]->keyDOWN(ev.keyboard.keycode);
                    else       players[myPlayerId]->keyUP(ev.keyboard.keycode);

                    players[myPlayerId]->updateMovingState();
                }
                else {
                    sendInputPacket(ev.keyboard.keycode, isDown);
                }
            }
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false;

            if (isServer) {
                for (auto& p : players) {
                    p->move();
                    p->updateMovingState();
                }

                for (auto& e : baseMap.entities) e->move();

                for (auto& e : baseMap.entities) {
                    e->collide(players);
                }

                for (int i = 0; i < players.size(); i++) {
                    StatePacket pkt;
                    pkt.type = PACKET_STATE;
                    pkt.id = i;
                    pkt.x = (float)players[i]->posX;
                    pkt.y = (float)players[i]->posY;
                    pkt.current_frame_y = players[i]->current_frame_y;
                    pkt.isMoving = players[i]->isMoving;

                    ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
                    enet_host_broadcast(netHost, 0, packet);
                }

                for (int i = 0; i < baseMap.entities.size(); i++) {
                    Car* car = (Car*)baseMap.entities[i];

                    StatePacket pkt;
                    pkt.type = PACKET_ENTITY_STATE;
                    pkt.id = i;
                    pkt.x = (float)car->posX;
                    pkt.y = (float)car->posY;

                    pkt.current_frame_y = car->movingLeft ? 1 : 0;
                    pkt.isMoving = true;

                    ENetPacket* packet = enet_packet_create(&pkt, sizeof(StatePacket), ENET_PACKET_FLAG_UNSEQUENCED);
                    enet_host_broadcast(netHost, 0, packet);
                }
            }

            al_clear_to_color(al_map_rgb(0, 0, 0));

            baseMap.drawMap();

            for (auto& e : baseMap.entities) e->draw();

            for (auto& p : players) p->draw();

            al_draw_textf(font, al_map_rgb(255, 255, 255), 10, 10, 0, "Sou Player: %d", myPlayerId);
            al_flip_display();
        }
    }

cleanup:
    if (netHost) enet_host_destroy(netHost);

    for (int i = 0; i < baseMap.tileNames.size(); i++)
        if (baseMap.tiles[i]) al_destroy_bitmap(baseMap.tiles[i]);

    for (auto& e : baseMap.entities) {
        e->destroy();
        delete e;
    }

    for (auto& p : players) {
        p->destroy();
        delete p;
    }

    al_destroy_display(display);
    al_destroy_font(font);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}