#include "src/levels/Level.h"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "src/managers/resource/ResourceManager.h"

using json = nlohmann::json;

Level::Level() {
    busSheet = nullptr;
    busFrameW = 0;
    busFrameH = 0;
    isLoaded = false;
    
    weatherSprite = nullptr;
    hasWeather = false;

    for (int i = 0; i < HMAP; i++)
        for (int j = 0; j < WMAP; j++)
            map[i][j] = 0;
}

Level::~Level() {
    for (auto e : entities) {
        if (e) { e->destroy(); delete e; }
    }
}

bool Level::loadMapFromJson(const std::string& jsonPath) {
    isLoaded = false;

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
        if (e) { e->destroy(); delete e; }
    }
    entities.clear();


    if (j.contains("title") && !j["title"].is_null())
        title = j["title"].get<std::string>();
    else title = "";

    if (j.contains("nextLevel") && !j["nextLevel"].is_null())
        nextLevelPath = j["nextLevel"].get<std::string>();
    else nextLevelPath = "";

    if (weatherSprite) { al_destroy_bitmap(weatherSprite); weatherSprite = nullptr; }
    hasWeather = false;

    // Agora procura por "weather" no JSON
    if (j.contains("weather")) {
        auto wData = j["weather"]; // wData = weatherData
        std::string wPath = wData["path"];
        
        weatherSprite = ResourceManager::get().getBitmap(wPath);
        
        if (weatherSprite) {
            hasWeather = true;
            weatherFrames = wData.value("frames", 1);
            weatherSpeed  = wData.value("speed", 10);
            
            // Cálculos
            weatherFrameW = al_get_bitmap_width(weatherSprite) / weatherFrames;
            weatherFrameH = al_get_bitmap_height(weatherSprite);
        }
    }
    // -------------------------------------

    if (j.contains("checkpoint")) {
        checkpoint.x = j["checkpoint"]["x"].get<int>() * BLOCKSIZE;
        checkpoint.y = j["checkpoint"]["y"].get<int>() * BLOCKSIZE;

        busSheet = ResourceManager::get().getBitmap("assets/sprites/cars/bus.png");

        if (busSheet) {
            busFrameW = al_get_bitmap_width(busSheet) / 2;
            busFrameH = al_get_bitmap_height(busSheet);
        }
        else {
            printf("[ERRO VISUAL] Nao achei a imagem do onibus!\n");
        }
    }

    if (j.contains("tiles")) {
        tileNames = j["tiles"].get<std::vector<std::string>>();
        for (size_t i = 0; i < tileNames.size(); i++) {
            tiles[i] = ResourceManager::get().getBitmap(tileNames[i]);

            if (!tiles[i]) {
                printf("[ERRO] Falha ao carregar tile: %s\n", tileNames[i].c_str());
            }
        }
    }

    tileProps.clear();

    if (!tileNames.empty()) {
        tileProps.resize(tileNames.size());
    }

    if (j.contains("tileProperties")) {
        for (auto& element : j["tileProperties"].items()) {
            int id = std::stoi(element.key());

            if (id >= 0 && id < tileProps.size()) {
                auto props = element.value();

                if (props.contains("solid")) tileProps[id].solid = props["solid"];
                if (props.contains("deadly")) tileProps[id].deadly = props["deadly"];
                if (props.contains("speedFactor")) tileProps[id].speedFactor = props["speedFactor"];
                if (props.contains("forceX")) tileProps[id].forceX = props["forceX"];
                if (props.contains("forceY")) tileProps[id].forceY = props["forceY"];
            }
        }
    }

    if (j.contains("entities") && j["entities"].is_array()) {
        for (auto& e : j["entities"]) {
            std::string type = e["type"];
            if (type == "car") {
                Car* car = new Car();
                if (e.contains("sprite")) car->spritePath = e["sprite"].get<std::string>();

                car->setPosX(e["posX"].get<int>());
                car->setPosY(e["posY"].get<int>());

                car->speed = e["speed"].get<float>();
                car->active = e["active"].get<bool>();

                if (e.contains("frameCount")) car->frameCount = e["frameCount"].get<int>();
                else car->frameCount = 1;

                if (e.contains("animated")) car->animated = e["animated"].get<bool>();

                std::string dir = e["direction"].get<std::string>();
                car->movingLeft = (dir == "left");

                car->reloadBitMap();
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

    isLoaded = true;
    return true;
}

void Level::updateWeather() {
    if (!hasWeather || weatherFrames <= 1) return;

    weatherTimer++;
    if (weatherTimer >= weatherSpeed) {
        weatherTimer = 0;
        weatherCurrentFrame++;
        
        if (weatherCurrentFrame >= weatherFrames) {
            weatherCurrentFrame = 0;
        }
        
    }
}

void Level::drawWeather() {
    if (!hasWeather || !weatherSprite) return;


    float sx = weatherCurrentFrame * weatherFrameW;

    for (int y = 0; y < SCREENHEIGHT; y += weatherFrameH) {
        
        for (int x = 0; x < SCREENWIDTH; x += weatherFrameW) {
            
            al_draw_bitmap_region(weatherSprite, 
                sx, 0,
                weatherFrameW, weatherFrameH,
                x, y, 0
            );
        }
    }
}

void Level::drawMap() {
    if (!isLoaded) return;

    for (int i = 0; i < HMAP; i++) {
        for (int j = 0; j < WMAP; j++) {
            int tileIndex = map[i][j];
            if (tileIndex >= 0 && tileIndex < (int)tileNames.size() && tiles[tileIndex]) {
                al_draw_bitmap(tiles[tileIndex], j * BLOCKSIZE, i * BLOCKSIZE, 0);
            }
        }
    }
}

void Level::drawBus(bool isDoorClosed) {
    if (busSheet && isLoaded) {
        int srcX = isDoorClosed ? busFrameW : 0;
        al_draw_bitmap_region(busSheet, srcX, 0, busFrameW, busFrameH, checkpoint.x, checkpoint.y, 0);
    }
}

bool Level::checkBusCollision(int px, int py, int pw, int ph) {
    if (!isLoaded) return false;
    int bx = checkpoint.x;
    int by = checkpoint.y;
    int bw = busFrameW;
    int bh = busFrameH;
    return (px < bx + bw && px + pw > bx && py < by + bh && py + ph > by);
}