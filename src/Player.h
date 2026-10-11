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

#include "raylib.h"

#include "Config.h"
#include "Map.h"

class Player {
public:
    Vector2 pos{1.5f, 1.5f};
    float angle = 0.0f;       // радианы; 0 = смотрим в +x
    float health = 100.0f;
    float armor = 0.0f;

    int   ammo[cfg::WEAPON_COUNT] = {0, 0, 0};      // патронов в магазине
    int   reserve[cfg::WEAPON_COUNT] = {0, 0, 0};   // запас (∞ у бесконечного оружия)
    float reloadTimer = 0.0f;                        // > 0 = перезаряжаемся

    float fireCooldown = 0.0f;
    float muzzleTimer = 0.0f;
    float damageFlash = 0.0f;
    int   kills = 0;
    int   weapon = 0;     // индекс в таблицах cfg::WEAPON_*

    // Базис камеры для рендера.
    Vector2 dir() const;      // куда смотрим (единичный)
    Vector2 plane() const;    // плоскость камеры, длина = tan(FOV/2)

    void spawn(Vector2 p, float a);
    void update(const Map& map, float dt);   // клавиатура + мышь
    void takeDamage(float dmg);

    bool startReload();                       // true, если перезарядка началась
    bool isReloading() const { return reloadTimer > 0.0f; }
    bool alive() const { return health > 0.0f; }

private:
    void finishReload();
};
