#pragma once
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <map>
#include <string>

class ResourceManager {
private:

    std::map<std::string, ALLEGRO_BITMAP*> bitmaps;
    std::map<std::string, ALLEGRO_FONT*> fonts;

    ResourceManager() {}

public:

    static ResourceManager& get() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    // --- BITMAPS ---
    ALLEGRO_BITMAP* getBitmap(const std::string& path) {
        if (bitmaps.find(path) != bitmaps.end()) {
            return bitmaps[path];
        }
        ALLEGRO_BITMAP* bmp = al_load_bitmap(path.c_str());
        if (bmp) {
            bitmaps[path] = bmp;
        }
        else {
            printf("[ERRO] Falha ao carregar imagem: %s\n", path.c_str());
        }
        return bmp;
    }

    // --- FONTES ---
    ALLEGRO_FONT* getFont(const std::string& path, int size) {
        std::string key = path + "_" + std::to_string(size);

        if (fonts.find(key) != fonts.end()) {
            return fonts[key];
        }

        ALLEGRO_FONT* font = al_load_font(path.c_str(), size, 0);
        if (font) {
            fonts[key] = font;
        }
        else {
            printf("[ERRO] Falha ao carregar fonte: %s\n", path.c_str());
        }
        return font;
    }

    void clear() {
        for (auto& pair : bitmaps) {
            al_destroy_bitmap(pair.second);
        }
        bitmaps.clear();

        for (auto& pair : fonts) {
            al_destroy_font(pair.second);
        }
        fonts.clear();
    }
};