#pragma once
#include "src/entities/players/Player.h"
class Sheep : public Player{
public:
	Sheep();
	virtual void updateDirection() override;
};

