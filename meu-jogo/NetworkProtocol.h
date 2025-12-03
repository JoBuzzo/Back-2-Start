#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

// Tipos de mensagens
enum PacketType {
    PACKET_INPUT,  // Cliente enviando tecla
    PACKET_STATE,   // Servidor enviando posiçoes
    PACKET_ENTITY_STATE,
    PACKET_WELCOME,
    PACKET_START_TRANSITION, // para começar a animar a saída
    PACKET_CHANGE_LEVEL // Carrega o mapa da proxima fase
};

// Pacote enviado do CLIENTE para o SERVIDOR
struct InputPacket {
    int type = PACKET_INPUT;
    int playerId; // Qual personagem o cliente controla
    int keycode;  // Tecla pressionada
    bool isDown;  // true = apertou, false = soltou
};

// Pacote enviado do SERVIDOR para o CLIENTE
struct StatePacket {
    int type;
    int id;
    float x;
    float y;

    // usar esse campo para passar a direçao do carro
    // Se for 1 = movingLeft true (Vira sprite)
    // Se for 0 = movingLeft false
    int current_frame_y;

    bool isMoving;
    bool isFinished;
};


struct WelcomePacket {
    int type = PACKET_WELCOME;
    int assignedId; // O ID que o servidor escolheu para o player
};
#endif