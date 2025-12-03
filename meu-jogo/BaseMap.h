#pragma once
#include "Config.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <string>
#include <vector>
#include "Car.h"

class BaseMap {
public:
    int map[HMAP][WMAP];
    std::vector<std::string> tileNames;
    std::vector<Car*> entities;
    ALLEGRO_BITMAP* tiles[20] = { nullptr };
    std::string path;

    BaseMap() = default;
    virtual ~BaseMap() = default;

    bool loadMapFromJson(const std::string& jsonPath);
    void drawMap();
};