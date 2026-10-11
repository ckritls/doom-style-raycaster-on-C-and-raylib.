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

// Рисует все текстуры на лету — ни одного файла-ассета. Звать после InitWindow().
class Textures {
public:
    void load();
    void unload();

    // walls()[type], type в 1..WALL_TYPE_COUNT (индекс 0 не используется).
    const std::vector<Texture2D>& walls() const { return walls_; }
    Texture2D enemy() const { return enemy_; }
    Texture2D healthPickup() const { return healthPickup_; }
    Texture2D armorPickup() const { return armorPickup_; }

private:
    std::vector<Texture2D> walls_;
    Texture2D enemy_{};
    Texture2D healthPickup_{};
    Texture2D armorPickup_{};
};
