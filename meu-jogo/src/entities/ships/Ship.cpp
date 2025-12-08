#include "src/entities/ships/Ship.h"

Ship::Ship() {
    frameCount = 12;   
    animated = true;

    hitboxOffsetLeft = 2;
    hitboxOffsetRight = 2;
    hitboxOffsetTop = 10;
    hitboxOffsetBottom = 2;
}

void Ship::load() {
    Entity::load(); 

    if (w > 0 && h > 0) {
        // --- ALTURA (Vertical) ---
        hitboxOffsetTop = (int)(h * 0.80);    // Corta 60% de cima (Velas)
        hitboxOffsetBottom = (int)(h * 0.10); // Corta 10% de baixo (Sombra)
        
        // Corta 20% da Direita (Bico do navio)
        hitboxOffsetRight = (int)(w * 0.20);  
        
        // Corta 5% da Esquerda (Traseira do navio)
        hitboxOffsetLeft = (int)(w * 0.05);   
    }
}