#pragma once
#include "Player.h"
class Bull : public Player {
public:
	char urlSprite[100] = "assets/sprites/players/bull.png";
	Bull();
	void reloadBitmap();
	void keyDOWN(int keycode) override;
	void keyUP(int keycode) override;
	void move() override;
	bool borderCollide();
};

