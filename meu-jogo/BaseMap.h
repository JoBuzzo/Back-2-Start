#pragma once
#include "Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <string>
#include <vector>
#include "Car.h"

struct Point { int x, y; };

class BaseMap {
public:
    std::string title;
    std::string nextLevelPath;

    Point checkpoint;
    ALLEGRO_BITMAP* busSheet;
    int busFrameW, busFrameH;
    std::vector<Point> spawnPoints;

    int map[HMAP][WMAP];
    std::vector<std::string> tileNames;
    std::vector<Car*> entities;
    ALLEGRO_BITMAP* tiles[20] = { nullptr };
    std::string path;

    BaseMap() { busSheet = nullptr; busFrameW = 0; busFrameH = 0; }
    virtual ~BaseMap() = default;

    bool loadMapFromJson(const std::string& jsonPath);
    void drawMap();
    void drawBus(bool isDoorClosed);
    bool checkBusCollision(int px, int py, int pw, int ph);
};