#pragma once
#include "src/entities/Entity.h"

class Ship : public Entity {
public:
    Ship();
    void load() override;
};