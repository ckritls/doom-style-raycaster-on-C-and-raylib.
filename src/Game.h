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
#include "Renderer.h"
#include "Textures.h"

// Короткоживущий трассер пули в экранных координатах.
struct Tracer {
    Vector2 start;
    Vector2 end;
    float life;
    float maxLife;
};

class Game {
public:
    Game();
    void run();

private:
    enum class State { Play, LevelClear, GameOver };

    void newLevel();          // генерирует свежий процедурный уровень
    void update(float dt);
    void draw();

    void handleWeaponSwitch();
    void switchWeapon(int index);
    void handleShooting();
    void shootRay(float angle, float damage, bool& anyHit, Vector2& endPoint);
    Vector2 projectPoint(Vector2 world) const;

    void updatePickups(float dt);
    void drawTracers();
    void drawHUD();
    void drawMinimap();
    void drawGun();
    void drawOverlay(const char* title, const char* subtitle, Color color);

    int runNumber_ = 0;   // +1 при каждой генерации уровня

    Map map_;
    Player player_;
    std::vector<Enemy> enemies_;
    std::vector<Pickup> pickups_;
    std::vector<Tracer> tracers_;
    Renderer renderer_;
    Textures textures_;

    State state_ = State::Play;
};
