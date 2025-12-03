#include "BaseMap.h"
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

bool BaseMap::loadMapFromJson(const std::string& jsonPath) {
    path = jsonPath;
    std::ifstream file(path);
    if (!file.is_open()) return false;

    json j;
    file >> j;

    tileNames = j["tiles"].get<std::vector<std::string>>();

    for (size_t i = 0; i < tileNames.size(); i++) {
        std::string filename = tileNames[i];
        tiles[i] = al_load_bitmap(filename.c_str());
        if (!tiles[i]) {
            printf("Erro ao carregar o tile: %s\n", filename.c_str());
        }
    }

    for (auto& e : j["entities"]) {
        std::string type = e["type"];
        if (type == "car") {
            Car* car = new Car();
            static std::string spritePathTemp;
            spritePathTemp = e["sprite"].get<std::string>();
            car->spritePath = spritePathTemp.c_str();
            car->setPosX(e["posX"]);
            car->setPosY(e["posY"]);
			car->w = e["w"];
			car->h = e["h"];
            car->speed = e["speed"];
            car->active = e["active"];
            car->movingLeft = (e["direction"] == "left");
            car->reloadBitMap();
            entities.push_back(car);
        }
    }

    auto mapData = j["map"];
    for (int i = 0; i < HMAP; i++)
        for (int j = 0; j < WMAP; j++)
            map[i][j] = mapData[i][j];

    return true;
}

void BaseMap::drawMap() {
    for (int i = 0; i < HMAP; i++) {
        for (int j = 0; j < WMAP; j++) {
            int tileIndex = map[i][j];
            if (tiles[tileIndex] != nullptr) {
                al_draw_bitmap(tiles[tileIndex], j * BLOCKSIZE, i * BLOCKSIZE, 0);
            }
        }
    }
}
