#include "Bull.h"

Bull::Bull() {
	w = 64;
	h = 64;
	frame = 1.f;
	current_frame_y = 0;
	posX = (WMAP * BLOCKSIZE / 2) - 16;
	posY = HMAP * BLOCKSIZE - 64;
	sprite = al_load_bitmap(urlSprite);
}

void Bull::reloadBitmap() {
	if (sprite) al_destroy_bitmap(sprite);
	sprite = al_load_bitmap(urlSprite);
}

void Bull::keyDOWN(int keycode) {
	switch (keycode) {
	case ALLEGRO_KEY_W: keys[W] = true; current_frame_y = 64; break;
	case ALLEGRO_KEY_S: keys[S] = true; current_frame_y = 0; break;
	case ALLEGRO_KEY_A: keys[A] = true; current_frame_y = 64 * 2; break;
	case ALLEGRO_KEY_D: keys[D] = true; current_frame_y = 64 * 3; break;
	}
}

void Bull::keyUP(int keycode) {
	switch (keycode) {
	case ALLEGRO_KEY_W: keys[W] = false; break;
	case ALLEGRO_KEY_S: keys[S] = false; break;
	case ALLEGRO_KEY_A: keys[A] = false; break;
	case ALLEGRO_KEY_D: keys[D] = false; break;
	}
}

bool Bull::borderCollide() {
	if (posX > WMAP * BLOCKSIZE - 32) { posX -= 2; return true; }
	if (posX < 0) { posX += 2; return true; }
	if (posY < 0) { posY += 2; return true; }
	if (posY > HMAP * BLOCKSIZE - 32) { posY -= 2; return true; }
	return false;
}

void Bull::move() {
	if (!borderCollide()) {
		if (keys[D]) posX += 2;
		if (keys[A]) posX -= 2;
		if (keys[S]) posY += 2;
		if (keys[W]) posY -= 2;
	}
}