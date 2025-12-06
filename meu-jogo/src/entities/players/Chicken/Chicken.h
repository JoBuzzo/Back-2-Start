#pragma once
#include "src/entities/players/Player.h"

class Chicken : public Player {
public:
    Chicken();
    virtual void updateDirection() override;
};