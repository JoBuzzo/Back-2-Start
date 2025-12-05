#pragma once
#include "src/core/Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <string>
#include <vector>
#include "src/entities/cars/Car.h"

struct Point { int x, y; };

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
    std::vector<Car*> entities;
    ALLEGRO_BITMAP* tiles[20] = { nullptr };

    Level();
    virtual ~Level();

    bool loadMapFromJson(const std::string& jsonPath);
    void drawMap();
    void drawBus(bool isDoorClosed);
    bool checkBusCollision(int px, int py, int pw, int ph);
};