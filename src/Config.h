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

// Все ручки настройки в одном месте. Крути и не благодари.
namespace cfg {

    // Окно и камера.
    constexpr int   SCREEN_WIDTH  = 1280;
    constexpr int   SCREEN_HEIGHT = 720;
    constexpr float FOV_DEGREES   = 66.0f;
    constexpr float FOV_SCALE     = 0.6494f;   // tan(FOV/2) — посчитал заранее, лень каждый кадр

    // Игрок.
    constexpr float MOVE_SPEED    = 4.2f;      // клеток в секунду
    constexpr float ROT_SPEED     = 2.4f;      // радиан/сек (клавиши)
    constexpr float MOUSE_SENS    = 0.0025f;   // радиан на пиксель мыши
    constexpr float PLAYER_RADIUS = 0.2f;      // радиус коллизии, в клетках
    constexpr float PLAYER_MAX_HP = 100.0f;
    constexpr float WEAPON_RANGE  = 100.0f;    // дальность hitscan
    constexpr float HITBOX_PAD    = 0.22f;     // щедрый запас — целиться можно криво

    // Оружие. Урон за одну дробинку: дробовик стреляет целым веером.
    constexpr int   WEAPON_COUNT = 3;
    constexpr const char* WEAPON_NAME[WEAPON_COUNT] = {"PISTOL", "SHOTGUN", "SMG"};
    constexpr float WEAPON_DAMAGE[WEAPON_COUNT]    = {34.0f, 14.0f, 10.0f};
    constexpr int   WEAPON_PELLETS[WEAPON_COUNT]   = {1, 6, 1};
    constexpr float WEAPON_COOLDOWN[WEAPON_COUNT]  = {0.28f, 0.85f, 0.09f};
    constexpr float WEAPON_SPREAD[WEAPON_COUNT]    = {0.0f, 0.10f, 0.035f}; // разброс, радианы
    constexpr bool  WEAPON_AUTO[WEAPON_COUNT]      = {false, false, true};

    // Патроны и перезарядка: магазин, время, бесконечный ли запас.
    constexpr int   WEAPON_MAG[WEAPON_COUNT]           = {12, 6, 30};
    constexpr float WEAPON_RELOAD_TIME[WEAPON_COUNT]   = {1.0f, 1.5f, 1.2f};
    constexpr bool  WEAPON_INFINITE[WEAPON_COUNT]      = {true, false, false};
    constexpr int   WEAPON_START_RESERVE[WEAPON_COUNT] = {0, 18, 90};

    // Враги.
    constexpr float ENEMY_HP              = 60.0f;
    constexpr float ENEMY_SPEED           = 1.6f;    // бегут, но не спешат
    constexpr float ENEMY_RADIUS          = 0.25f;
    constexpr float ENEMY_ATTACK_RANGE    = 6.0f;
    constexpr float ENEMY_ATTACK_COOLDOWN = 1.1f;
    constexpr float ENEMY_DAMAGE          = 8.0f;
    constexpr float ENEMY_REPATH_INTERVAL = 0.3f;    // как часто пересчитываем путь
    constexpr float ENEMY_SEPARATION_RADIUS = 0.7f;  // в этом радиусе расталкиваемся
    constexpr float ENEMY_SEPARATION_WEIGHT = 1.2f;  // насколько сильно расталкиваемся
    constexpr float ENEMY_OPEN_SPACE_PENALTY = 2.0f; // штраф за открытое пространство

    // Броня гасит часть урона, остальное уходит в хп.
    constexpr float PLAYER_MAX_ARMOR = 100.0f;
    constexpr float ARMOR_ABSORB      = 0.5f;

    // Пикапы.
    constexpr float HEALTH_PICKUP_AMOUNT  = 25.0f;
    constexpr float ARMOR_PICKUP_AMOUNT   = 25.0f;
    constexpr float PICKUP_COLLECT_RADIUS = 0.6f;

    // Трассеры.
    constexpr float TRACER_LIFE = 0.12f;

    // Спавн.
    constexpr float MIN_ENEMY_SPAWN_DIST = 6.0f;   // враги не лезут ближе этого

    // Текстуры.
    constexpr int   TEX_SIZE        = 64;
    constexpr int   WALL_TYPE_COUNT = 4;           // процедурных текстур стен

    // Миникарта.
    constexpr int   MINIMAP_CELL = 6;              // пикселей на клетку
}
