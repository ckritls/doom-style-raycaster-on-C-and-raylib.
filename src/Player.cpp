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
#include "Player.h"

#include <algorithm>
#include <cmath>

#include "Audio.h"

Vector2 Player::dir() const
{
    return {std::cos(angle), std::sin(angle)};
}

Vector2 Player::plane() const
{
    return {-std::sin(angle) * cfg::FOV_SCALE, std::cos(angle) * cfg::FOV_SCALE};
}

void Player::spawn(Vector2 p, float a)
{
    pos = p;
    angle = a;
    health = cfg::PLAYER_MAX_HP;
    armor = 0.0f;
    reloadTimer = 0.0f;

    for (int i = 0; i < cfg::WEAPON_COUNT; ++i) {
        ammo[i] = cfg::WEAPON_MAG[i];
        reserve[i] = cfg::WEAPON_START_RESERVE[i];
    }

    fireCooldown = 0.0f;
    muzzleTimer = 0.0f;
    damageFlash = 0.0f;
    kills = 0;
}

void Player::update(const Map& map, float dt)
{
    fireCooldown = std::max(0.0f, fireCooldown - dt);
    muzzleTimer = std::max(0.0f, muzzleTimer - dt);
    damageFlash = std::max(0.0f, damageFlash - dt);

    // Докручиваем перезарядку.
    if (reloadTimer > 0.0f) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            reloadTimer = 0.0f;
            finishReload();
        }
    }

    const Vector2 d = dir();

    // --- Поворот: A/D или стрелки, плюс мышь (только по горизонтали)
    float keyRot = 0.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) keyRot -= cfg::ROT_SPEED;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) keyRot += cfg::ROT_SPEED;

    angle += keyRot * dt;
    angle += GetMouseDelta().x * cfg::MOUSE_SENS;

    // --- Движение: W/S или стрелки вверх/вниз
    float move = 0.0f;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) move += cfg::MOVE_SPEED;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) move -= cfg::MOVE_SPEED;

    // Двигаем по осям раздельно, чтобы не цепляться за углы.
    const float nx = pos.x + d.x * move * dt;
    const float ny = pos.y + d.y * move * dt;
    if (!map.collides(nx, pos.y, cfg::PLAYER_RADIUS)) pos.x = nx;
    if (!map.collides(pos.x, ny, cfg::PLAYER_RADIUS)) pos.y = ny;
}

void Player::takeDamage(float dmg)
{
    if (health <= 0.0f) return;

    // Броня берёт на себя часть удара, остальное — по хп.
    const float absorbed = std::min(armor, dmg * cfg::ARMOR_ABSORB);
    armor -= absorbed;
    health -= (dmg - absorbed);

    damageFlash = 0.35f;
    audio::playPlayerHurt();
    if (health < 0.0f) health = 0.0f;
}

bool Player::startReload()
{
    if (isReloading()) return false;
    if (ammo[weapon] >= cfg::WEAPON_MAG[weapon]) return false;
    if (!cfg::WEAPON_INFINITE[weapon] && reserve[weapon] <= 0) return false;

    reloadTimer = cfg::WEAPON_RELOAD_TIME[weapon];
    audio::playReload();
    return true;
}

void Player::finishReload()
{
    const int mag = cfg::WEAPON_MAG[weapon];
    if (cfg::WEAPON_INFINITE[weapon]) {
        ammo[weapon] = mag;
    } else {
        const int need = mag - ammo[weapon];
        const int take = std::min(need, reserve[weapon]);
        ammo[weapon] += take;
        reserve[weapon] -= take;
    }
}
