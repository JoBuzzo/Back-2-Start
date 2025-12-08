#pragma once
#include "src/core/Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <string>
#include <vector>

// IMPORTANTE: O Level precisa conhecer o Entity completo aqui
#include "src/entities/Entity.h"

struct Point { int x, y; };
struct TileData {
    bool solid = false;
    bool deadly = false;
    float speedFactor = 1.0f;
    float forceX = 0.0f;
    float forceY = 0.0f;
};

class Level {
public:
    bool isLoaded;
    std::string title;
    std::string nextLevelPath;
    std::string path;

    Point checkpoint;
    ALLEGRO_BITMAP* busSheet;
    int busFrameW, busFrameH;

    std::vector<Point> spawnPoints;

    int map[HMAP][WMAP];
    std::vector<std::string> tileNames;
    
    std::vector<Entity*> entities; 
    
    ALLEGRO_BITMAP* tiles[20] = { nullptr };
    std::vector<TileData> tileProps;

    ALLEGRO_BITMAP* weatherSprite;
    bool hasWeather;
    int weatherFrames;
    int weatherCurrentFrame;
    int weatherSpeed;
    int weatherTimer;
    int weatherFrameW;
    int weatherFrameH;

    Level();
    virtual ~Level();

    bool loadMapFromJson(const std::string& jsonPath);
    void drawMap();
    void drawBus(bool isDoorClosed);
    bool checkBusCollision(int px, int py, int pw, int ph);
    void updateWeather();
    void drawWeather();
    
    TileData getTileProp(int id) {
        if (id >= 0 && id < tileProps.size()) return tileProps[id];
        return TileData();
    }
    
    int getTileIdAt(int x, int y) {
        int col = x / BLOCKSIZE;
        int row = y / BLOCKSIZE;
        if (col < 0 || col >= WMAP || row < 0 || row >= HMAP) return 0;
        return map[row][col];
    }
    
    void reset();
};