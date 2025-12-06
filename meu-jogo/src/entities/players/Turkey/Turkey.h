#pragma once
#include "src/entities/players/Player.h"
class Turkey : public Player {
public:
	Turkey();
	void updateDirection() override;
};

