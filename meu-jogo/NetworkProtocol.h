#pragma once

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
    int playerId = -1;
    int keycode = 0;
    bool isDown = false;
};

struct StatePacket {
    int type = PACKET_STATE;
    int id = -1;
    float x = 0.0f;
    float y = 0.0f;
    int current_frame_y = 0;
    bool isMoving = false;
    bool isFinished = false;
};

struct WelcomePacket {
    int type = PACKET_WELCOME;
    int assignedId = -1;
    char currentLevelPath[128] = { 0 };
};

struct PacketHeader {
    int type;
};