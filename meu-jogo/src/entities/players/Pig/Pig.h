#pragma once
#include "src/entities/players/Player.h"
class Pig : public Player{
	public:
	Pig();
	virtual void updateDirection() override;
};

