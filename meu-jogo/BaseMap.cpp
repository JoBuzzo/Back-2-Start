#include "BaseMap.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool BaseMap::loadMapFromJson(const std::string& jsonPath) {
    path = jsonPath;
    std::ifstream file(path);
    if (!file.is_open()) {
        printf("[ERRO] Nao foi possivel abrir o arquivo: %s\n", path.c_str());
        return false;
    }

    json j;
    try {
        file >> j;
    }
    catch (const std::exception& e) {
        printf("[ERRO] JSON invalido ou corrompido em %s: %s\n", path.c_str(), e.what());
        return false;
    }

    for (auto e : entities) {
        if (e) {
            e->destroy();
            delete e;
        }
    }
    entities.clear();


    if (j.contains("title") && !j["title"].is_null())
        title = j["title"].get<std::string>();
    else
        title = "";

    if (j.contains("nextLevel") && !j["nextLevel"].is_null())
        nextLevelPath = j["nextLevel"].get<std::string>();
    else
        nextLevelPath = "";

    if (j.contains("checkpoint")) {
        checkpoint.x = j["checkpoint"]["x"].get<int>() * BLOCKSIZE;
        checkpoint.y = j["checkpoint"]["y"].get<int>() * BLOCKSIZE;

        if (!busSheet) {
            busSheet = al_load_bitmap("assets/sprites/cars/bus.png");
            if (busSheet) {
                busFrameW = al_get_bitmap_width(busSheet) / 2;
                busFrameH = al_get_bitmap_height(busSheet);
            }
            else {
                printf("[AVISO] Imagem 'assets/sprites/cars/bus.png' nao encontrada!\n");
            }
        }
    }

    if (j.contains("tiles")) {
        tileNames = j["tiles"].get<std::vector<std::string>>();
        for (size_t i = 0; i < tileNames.size(); i++) {
            if (tiles[i]) al_destroy_bitmap(tiles[i]);
            tiles[i] = al_load_bitmap(tileNames[i].c_str());
            if (!tiles[i]) printf("[ERRO] Tile nao encontrado: %s\n", tileNames[i].c_str());
        }
    }

    if (j.contains("entities") && j["entities"].is_array()) {
        for (auto& e : j["entities"]) {
            std::string type = e["type"];
            if (type == "car") {
                Car* car = new Car();

                if (e.contains("sprite") && !e["sprite"].is_null())
                    car->spritePath = e["sprite"].get<std::string>();
                else
                    car->spritePath = "assets/sprites/cars/fusca.png";

                if (e.contains("animated"))
                    car->animated = e["animated"].get<bool>();
                else
                    car->animated = false;

                if (e.contains("frameCount"))
                    car->frameCount = e["frameCount"].get<int>();
                else
                    car->frameCount = 1;

                car->setPosX(e["posX"].get<int>());
                car->setPosY(e["posY"].get<int>());
                car->speed = e["speed"].get<float>();
                car->active = e["active"].get<bool>();

                std::string dir = e["direction"].get<std::string>();
                car->movingLeft = (dir == "left");

                car->reloadBitMap();
                if (!car->sprite) printf("[ERRO] Sprite falhou: %s\n", car->spritePath.c_str());

                entities.push_back(car);
            }
        }
    }

    spawnPoints.clear();
    if (j.contains("spawns")) {
        for (auto& s : j["spawns"]) {
            spawnPoints.push_back({ s["x"].get<int>() * BLOCKSIZE, s["y"].get<int>() * BLOCKSIZE });
        }
    }

    if (j.contains("map")) {
        auto mapData = j["map"];
        int rows = std::min((int)mapData.size(), HMAP);
        int cols = std::min((int)mapData[0].size(), WMAP);

        for (int i = 0; i < rows; i++)
            for (int j = 0; j < cols; j++)
                map[i][j] = mapData[i][j];
    }

    return true;
}


void BaseMap::drawMap() {
    for (int i = 0; i < HMAP; i++) {
        for (int j = 0; j < WMAP; j++) {
            int tileIndex = map[i][j];
            if (tileIndex >= 0 && tileIndex < tileNames.size() && tiles[tileIndex] != nullptr) {
                al_draw_bitmap(tiles[tileIndex], j * BLOCKSIZE, i * BLOCKSIZE, 0);
            }
        }
    }
}

void BaseMap::drawBus(bool isDoorClosed) {
    if (busSheet) {
        int srcX = isDoorClosed ? busFrameW : 0;
        al_draw_bitmap_region(busSheet, srcX, 0, busFrameW, busFrameH, checkpoint.x, checkpoint.y, 0);
    }
}

bool BaseMap::checkBusCollision(int px, int py, int pw, int ph) {
    int bx = checkpoint.x;
    int by = checkpoint.y;
    int bw = busFrameW;
    int bh = busFrameH;

    return (px < bx + bw && px + pw > bx && py < by + bh && py + ph > by);
}