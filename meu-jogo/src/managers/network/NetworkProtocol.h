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
    int type;
    int id;
    float x;
    float y;
    int current_frame_y;
    bool isMoving;
    bool isRunning;
    bool isFinished;
    float z;
    bool isJumping;
};

struct WelcomePacket {
    int type = PACKET_WELCOME;
    int assignedId = -1;
    char currentLevelPath[128] = { 0 };
};

struct PacketHeader {
    int type;
};