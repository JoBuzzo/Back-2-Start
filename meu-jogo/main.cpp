#include "Config.h"
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_font.h>

#include <ostream>
#include "BaseMap.h"
#include "Chicken.h"
#include "Car.h"

int main() {
    al_init();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_primitives_addon();
    al_init_image_addon();
    al_install_keyboard();

    // al_set_new_display_flags(ALLEGRO_FULLSCREEN);
    ALLEGRO_DISPLAY* display = al_create_display(SCREENWIDTH, SCREENHEIGHT);
    al_set_window_title(display, "Street Tile!");

    ALLEGRO_MONITOR_INFO info;
    al_get_monitor_info(0, &info);
    int displayWidth = info.x2 - info.x1;
    int displayHeight = info.y2 - info.y1;
    int posX = (displayWidth - SCREENWIDTH) / 2;
    int posY = (displayHeight - SCREENHEIGHT) / 2;
    al_set_window_position(display, posX, posY);

    ALLEGRO_FONT* font = al_load_font("assets/fonts/font.ttf", 25, 0);
    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 90.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_start_timer(timer);

    BaseMap baseMap;
    if (!baseMap.loadMapFromJson("assets/maps/level1.json")) {
        printf("Erro ao carregar o mapa!\n");
        return -1;
    }

    Chicken player;
    bool redraw = true;

    while (true) {
        ALLEGRO_EVENT ev;
        while (al_get_next_event(queue, &ev)) {
            if (ev.type == ALLEGRO_EVENT_TIMER) {
                redraw = true;
            }
            else if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
                return 0;
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_DOWN) {
                player.keyDOWN(ev.keyboard.keycode);
            }
            else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
                player.keyUP(ev.keyboard.keycode);
            }
        }

        if (redraw && al_is_event_queue_empty(queue)) {
            redraw = false;

            player.move();

            for (auto& e : baseMap.entities) {
                e->move();
                e->collide(player);
            }

            al_clear_to_color(al_map_rgb(0, 0, 0));
            baseMap.drawMap();

            for (auto& e : baseMap.entities) {
                e->draw();
            }

            player.draw();

            al_draw_text(font, al_map_rgb(255, 255, 255), BLOCKSIZE * 35, 7, 0, "Fase 1");

            al_flip_display();
        }
    }

    for (int i = 0; i < baseMap.tileNames.size(); i++)
        if (baseMap.tiles[i]) al_destroy_bitmap(baseMap.tiles[i]);

    for (auto& e : baseMap.entities) {
        e->destroy();
        delete e;
    }

    al_destroy_display(display);
}
