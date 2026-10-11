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

#include <string>
#include <vector>

#include "raylib.h"

// Точка спавна пикапа из сетки ('H' = аптечка, 'A' = броня).
struct PickupSpawn {
    Vector2 pos;
    int type;   // 0 = аптечка, 1 = броня
};

// Сетка уровня, 1 клетка = 1 мировая единица.
//  '#' / '1'..'9'               -> стена (цифра выбирает текстуру)
//  ' ', '.', 'P', 'E', 'H', 'A' -> пол / точки спавна
class Map {
public:
    void load(const std::vector<std::string>& layout);

    int width() const { return width_; }
    int height() const { return height_; }

    bool isWall(int x, int y) const;          // за границей тоже «стена»
    int  wallType(int x, int y) const;        // 0 = пусто, 1..9 = текстура
    bool collides(float x, float y, float radius) const;
    bool lineOfSight(float x0, float y0, float x1, float y1) const;

    // Расстояние до первой стены вдоль единичного луча; -1 если упёрлись в пустоту.
    float raycast(float x, float y, float dirX, float dirY, Vector2& hitPoint) const;

    // A*: кратчайший путь с учётом «цены открытого пространства» (враги жмутся
    // к стенам). Кладёт в `path` центры клеток (старт и цель включительно).
    // false, если пути нет.
    bool findPath(int sx, int sy, int gx, int gy, std::vector<Vector2>& path) const;

    Vector2 playerStart() const { return playerStart_; }
    float   playerAngle() const { return playerAngle_; }
    const std::vector<Vector2>& enemyStarts() const { return enemyStarts_; }
    const std::vector<PickupSpawn>& pickupStarts() const { return pickupStarts_; }

private:
    void computeWallDistance();             // дистанция до ближайшей стены (для A*)
    float cellCost(int x, int y) const;     // чем дальше от стены, тем дороже

    std::vector<std::string> grid_;
    std::vector<int> wallDist_;
    int width_ = 0;
    int height_ = 0;
    Vector2 playerStart_{1.5f, 1.5f};
    float playerAngle_ = 0.0f;
    std::vector<Vector2> enemyStarts_;
    std::vector<PickupSpawn> pickupStarts_;
};
