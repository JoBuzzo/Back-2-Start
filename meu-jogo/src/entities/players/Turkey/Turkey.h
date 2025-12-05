#pragma once
#include "src/entities/players/Player.h"
class Turkey : public Player {
public:
	char urlSprite[100] = "assets/sprites/players/turkey.png";
	Turkey();

	void reloadBitmap();
	void keyDOWN(int keycode) override;
	void keyUP(int keycode) override;
	void move() override;
	bool borderCollide();
};

