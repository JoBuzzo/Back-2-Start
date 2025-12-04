#ifndef NETWORKPROTOCOL_H
#define NETWORKPROTOCOL_H

#include <cstring>

enum PacketType {
    PACKET_INPUT,
    PACKET_STATE,
    PACKET_ENTITY_STATE,
    PACKET_WELCOME,
    PACKET_START_TRANSITION,
    PACKET_CHANGE_LEVEL
};

struct InputPacket {
    int type = PACKET_INPUT;
    int playerId;
    int keycode;
    bool isDown;
};

struct StatePacket {
    int type = PACKET_STATE;
    int id;
    float x;
    float y;
    int current_frame_y;
    bool isMoving;
    bool isFinished;
};

struct WelcomePacket {
    int type = PACKET_WELCOME;
    int assignedId;
    char currentMap[64];
};

#endif