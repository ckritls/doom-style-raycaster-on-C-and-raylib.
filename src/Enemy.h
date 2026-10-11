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

#include "Map.h"

class Player;

// Бес-навигатор: всегда знает, где игрок, прокладывает путь (A*) вдоль стен,
// избегает открытых площадок и не кучкуется с собратьями. Кусается, когда видит.
class Enemy {
public:
    explicit Enemy(Vector2 p);

    Vector2 pos;
    float health;
    float attackCooldown = 0.0f;
    float hitFlash = 0.0f;

    bool alive() const { return health > 0.0f; }
    void takeDamage(float dmg);
    void update(const Map& map, Player& player, const std::vector<Enemy>& enemies, float dt);

private:
    void recomputePath(const Map& map, int gx, int gy);
    Vector2 nextTarget(const Player& player);
    void moveToward(const Map& map, const std::vector<Enemy>& enemies, Vector2 target, float dt);

    std::vector<Vector2> path_;   // центры клеток маршрута
    int pathIndex_ = 0;
    float repathTimer_ = 0.0f;
    int lastTargetX_ = -1;
    int lastTargetY_ = -1;
};
