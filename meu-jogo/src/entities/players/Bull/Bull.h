#pragma once
#include "src/entities/players/Player.h"
class Bull : public Player
{
public:
	Bull();
	virtual void updateDirection() override;
};
