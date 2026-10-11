/*
 * Copyright (C) 2026 ckritls
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even meven implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#pragma once

#include <vector>

#include "raylib.h"

#include "Enemy.h"
#include "Map.h"
#include "Pickup.h"
#include "Player.h"
#include "Textures.h"

// Рейкастер в духе Doom: текстурированные стены + билборды (враги и пикапы)
// с z-буфером на каждую колонку.
class Renderer {
public:
    void render(const Map& map, const Player& player,
                const std::vector<Enemy>& enemies, const std::vector<Pickup>& pickups,
                const Textures& textures);

private:
    void renderWalls(const Map& map, const Player& player, const Textures& textures);
    void renderSprites(const Player& player, const std::vector<Enemy>& enemies,
                       const std::vector<Pickup>& pickups, const Textures& textures);

    std::vector<float> zbuffer_;
};
