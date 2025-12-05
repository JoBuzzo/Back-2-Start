#include "src/managers/network/NetworkManager.h"
#include <iostream>
#include <cstring>

NetworkManager::NetworkManager() {
    host = nullptr;
    serverPeer = nullptr;
    _isServer = false;
    _myPlayerId = -1;
    _nextPlayerId = 1;
}

NetworkManager::~NetworkManager() {
    shutdown();
}

bool NetworkManager::init() {
    if (enet_initialize() != 0) {
        printf("[NET ERROR] Failed to initialize ENet.\n");
        return false;
    }
    return true;
}

void NetworkManager::shutdown() {
    disconnect();
    enet_deinitialize();
}

bool NetworkManager::startHost(int port) {
    _isServer = true;
    _myPlayerId = 0;   // Host é sempre 0
    _nextPlayerId = 1; // Próximo será 1

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    host = enet_host_create(&address, 32, 2, 0, 0); // 32 clientes, 2 canais

    if (host == nullptr) {
        printf("[NET ERROR] Failed to create host.\n");
        return false;
    }
    printf("[NET] Server started on port %d\n", port);
    return true;
}

bool NetworkManager::startClient(std::string ip, int port) {
    _isServer = false;
    _myPlayerId = -1; // Ainda não sei quem sou

    host = enet_host_create(NULL, 1, 2, 0, 0);
    if (host == nullptr) return false;

    ENetAddress address;
    enet_address_set_host(&address, ip.c_str());
    address.port = port;

    // Inicia conexão
    serverPeer = enet_host_connect(host, &address, 2, 0);

    if (serverPeer == nullptr) {
        printf("[NET ERROR] No available peers for initiating an ENet connection.\n");
        return false;
    }

    // Espera até 5 segundos para confirmar a conexão técnica
    ENetEvent event;
    if (enet_host_service(host, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        printf("[NET] Connection to %s:%d succeeded.\n", ip.c_str(), port);
        return true;
    }
    else {
        enet_peer_reset(serverPeer);
        printf("[NET] Connection to %s:%d failed.\n", ip.c_str(), port);
        return false;
    }
}

void NetworkManager::disconnect() {
    if (serverPeer) {
        enet_peer_disconnect_now(serverPeer, 0);
        serverPeer = nullptr;
    }
    if (host) {
        enet_host_destroy(host);
        host = nullptr;
    }
    _isServer = false;
    _myPlayerId = -1;
}

void NetworkManager::sendPacket(ENetPeer* peer, void* data, size_t size, bool reliable) {
    // Se peer for nulo e somos cliente, assume envio para o servidor
    if (peer == nullptr && !_isServer) peer = serverPeer;
    if (peer == nullptr) return;

    ENetPacket* packet = enet_packet_create(data, size, reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
    enet_peer_send(peer, 0, packet);
}

void NetworkManager::broadcastPacket(void* data, size_t size, bool reliable) {
    if (!host) return;
    ENetPacket* packet = enet_packet_create(data, size, reliable ? ENET_PACKET_FLAG_RELIABLE : ENET_PACKET_FLAG_UNSEQUENCED);
    enet_host_broadcast(host, 0, packet);
}

void NetworkManager::flush() {
    if (host) enet_host_flush(host);
}

// O Coração do Network Manager
void NetworkManager::update(std::function<void(ENetPeer*)> onConnect,
    std::function<void(ENetPeer*)> onDisconnect,
    std::function<void(ENetEvent&)> onReceive)
{
    if (!host) return;

    ENetEvent event;
    // Processa todos os eventos pendentes
    while (enet_host_service(host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            if (onConnect) onConnect(event.peer);
            break;

        case ENET_EVENT_TYPE_RECEIVE:
            if (onReceive) onReceive(event);
            enet_packet_destroy(event.packet);
            break;

        case ENET_EVENT_TYPE_DISCONNECT:
            if (onDisconnect) onDisconnect(event.peer);
            break;
        }
    }
}