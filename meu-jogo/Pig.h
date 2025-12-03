#pragma once
#include "Player.h"
class Pig : public Player{
	public:
	char urlSprite[100] = "assets/sprites/players/pig.png";
	Pig();
	void reloadBitmap();
	void keyDOWN(int keycode) override;
	void keyUP(int keycode) override;
	void move() override;
	bool borderCollide();
};

