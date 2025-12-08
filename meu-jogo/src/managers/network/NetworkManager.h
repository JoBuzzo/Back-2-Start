#pragma once
#include <enet/enet.h>
#include <string>
#include <functional>
#include "NetworkProtocol.h"

class NetworkManager {
private:
    ENetHost* host;
    ENetPeer* serverPeer; // Usado apenas se formos Cliente (para falar com o server)

    bool _isServer;
    int _myPlayerId;
    int _nextPlayerId; // Apenas Servidor usa

public:
    NetworkManager();
    ~NetworkManager();

    // Inicializacao da biblioteca
    bool init();
    void shutdown();

    // Conexao
    bool startHost(int port);
    bool startClient(std::string ip, int port);
    void disconnect();

    // Getters
    bool isServer() const { return _isServer; }
    int getMyId() const { return _myPlayerId; }
    void setMyId(int id) { _myPlayerId = id; }
    int getNextId() const { return _nextPlayerId; }
    void incrementNextId() { _nextPlayerId++; }

    // Envio de Dados
    // Envia para um peer especifico (ou para o servidor se for cliente)
    void sendPacket(ENetPeer* peer, void* data, size_t size, bool reliable);

    // Envia para todos (Broadcast)
    void broadcastPacket(void* data, size_t size, bool reliable);

    // Envia imediatamente (sem esperar o proximo loop)
    void flush();

    // --- O LOOP PRINCIPAL ---
    // Aceita 3 funcoes (callbacks) para rodar quando eventos acontecerem
    void update(
        std::function<void(ENetPeer*)> onConnect,
        std::function<void(ENetPeer*)> onDisconnect,
        std::function<void(ENetEvent&)> onReceive
    );
};