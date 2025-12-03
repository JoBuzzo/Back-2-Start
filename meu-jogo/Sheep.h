#pragma once
#include "Player.h"
class Sheep : public Player{
public:
	char urlSprite[100] = "assets/sprites/players/sheep.png";
	Sheep();
	void reloadBitmap();
	void keyDOWN(int keycode) override;
	void keyUP(int keycode) override;
	void move() override;
	bool borderCollide();
};

